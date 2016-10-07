/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/
#include <sstream>

#include <boost/algorithm/string.hpp>

#include <qi/log.hpp>

#include "messagesocket.hpp"
#include "transportsocketcache.hpp"

qiLogCategory("qimessaging.transportsocketcache");

namespace qi
{
TransportSocketCache::TransportSocketCache()
  : _dying(false)
{
}

TransportSocketCache::~TransportSocketCache()
{
  _dying = true;
  destroy();
  close();
}

void TransportSocketCache::init()
{
  _dying = false;
}

void TransportSocketCache::close()
{
  qiLogDebug() << "TransportSocketCache is closing";
  ConnectionMap map;
  std::list<MessageSocketPtr> pending;
  {
    boost::mutex::scoped_lock lock(_socketMutex);
    _dying = true;
    std::swap(map, _connections);
    std::swap(pending, _allPendingConnections);
  }
  for (ConnectionMap::iterator mIt = map.begin(), mEnd = map.end(); mIt != mEnd; ++mIt)
  {
    for (std::map<Url, ConnectionAttemptPtr>::iterator uIt = mIt->second.begin(), uEnd = mIt->second.end();
         uIt != uEnd;
         ++uIt)
    {
      auto& connectionAttempt = *uIt->second;
      MessageSocketPtr endpoint = connectionAttempt.endpoint;

      // Disconnect any valid socket we were holding.
      if (endpoint)
      {
        endpoint->disconnect();
        endpoint->disconnected.disconnect(connectionAttempt.disconnectionTracking);
      }
      else
      {
        uIt->second->state = State_Error;
        uIt->second->promise.setError("TransportSocketCache is closing.");
      }
    }
  }
  for (std::list<MessageSocketPtr>::iterator it = pending.begin(), end = pending.end(); it != end; ++it)
    (*it)->disconnect();
  _connections.clear();
}

bool isLocalHost(const std::string& host)
{
  return boost::algorithm::starts_with(host, "127.") || host == "localhost";
}

static UrlVector localhost_only(const UrlVector& input)
{
  UrlVector result;
  result.reserve(input.size());
  for (const auto& url: input)
  {
    if (isLocalHost(url.host()))
      result.push_back(url);
  }
  return result;
}

Future<MessageSocketPtr> TransportSocketCache::socket(const ServiceInfo& servInfo, const std::string& url)
{
  const std::string& machineId = servInfo.machineId();
  ConnectionAttemptPtr couple = boost::make_shared<ConnectionAttempt>();
  couple->relatedUrls = servInfo.endpoints();
  bool local = machineId == os::getMachineId();
  UrlVector connectionCandidates;

  // If the connection is local, we're mainly interested in localhost endpoint
  if (local)
    connectionCandidates = localhost_only(servInfo.endpoints());

  // If the connection isn't local or if the service doesn't expose local endpoints,
  // try and connect to whatever is available.
  if (connectionCandidates.size() == 0)
    connectionCandidates = servInfo.endpoints();

  couple->endpoint = MessageSocketPtr();
  couple->state = State_Pending;
  {
    // If we already have a pending connection to one of the urls, we return the future in question
    boost::mutex::scoped_lock lock(_socketMutex);

    if (_dying)
      return makeFutureError<MessageSocketPtr>("TransportSocketCache is closed.");

    ConnectionMap::iterator machineIt = _connections.find(machineId);
    if (machineIt != _connections.end())
    {
      // Check if any connection to the machine matches one of our urls
      UrlVector& vurls = couple->relatedUrls;
      for (std::map<Url, ConnectionAttemptPtr>::iterator b = machineIt->second.begin(), e = machineIt->second.end();
           b != e;
           b++)
      {
        UrlVector::iterator uIt = std::find(vurls.begin(), vurls.end(), b->first);
        // We found a matching machineId and URL : return the connected endpoint.
        if (uIt != vurls.end())
          return b->second->promise.future();
      }
    }
    // Otherwise, we keep track of all those URLs and assign them the same promise in our map.
    // They will all track the same connection.
    couple->attemptCount = connectionCandidates.size();
    std::map<Url, ConnectionAttemptPtr>& urlMap = _connections[machineId];
    for (const auto& url: connectionCandidates)
    {
      if (!url.isValid())
        continue; // Do not try to connect to an invalid url!

      if (!local && isLocalHost(url.host()))
        continue; // Do not try to connect on localhost when it is a remote!

      urlMap[url] = couple;
      MessageSocketPtr socket = makeMessageSocket(url.protocol());
      _allPendingConnections.push_back(socket);
      Future<void> sockFuture = socket->connect(url);
      qiLogDebug() << "Inserted [" << machineId << "][" << url.str() << "]";
      sockFuture.connect(&TransportSocketCache::onSocketParallelConnectionAttempt, this, _1, socket, url, servInfo);
    }
  }
  return couple->promise.future();
}

void TransportSocketCache::insert(const std::string& machineId, const Url& url, MessageSocketPtr socket)
{
  // If a connection is pending for this machine / url, terminate the pendage and set the
  // service socket as this one
  boost::mutex::scoped_lock lock(_socketMutex);

  if (_dying)
    return;

  ServiceInfo info;

  info.setMachineId(machineId);
  qi::SignalLink disconnectionTracking = socket->disconnected.connect(
        &TransportSocketCache::onSocketDisconnected, this, url, info)
      .setCallType(MetaCallType_Direct);

  ConnectionMap::iterator mIt = _connections.find(machineId);
  if (mIt != _connections.end())
  {
    std::map<Url, ConnectionAttemptPtr>::iterator uIt = mIt->second.find(url);
    if (uIt != mIt->second.end())
    {
      auto& connectionAttempt = *uIt->second;
      QI_ASSERT(!connectionAttempt.endpoint);
      // If the attempt is done and the endpoint is null, it means the
      // attempt failed and the promise is set as error.
      // We replace it by a new one.
      // If the attempt is not done we do not replace it, otherwise the future
      // currently in circulation will never finish.
      if (connectionAttempt.state != State_Pending)
        connectionAttempt.promise = Promise<MessageSocketPtr>();
      connectionAttempt.state = State_Connected;
      connectionAttempt.endpoint = socket;
      connectionAttempt.promise.setValue(socket);
      connectionAttempt.disconnectionTracking = disconnectionTracking;
      return;
    }
  }
  ConnectionAttemptPtr couple = boost::make_shared<ConnectionAttempt>();
  couple->promise = Promise<MessageSocketPtr>();
  couple->endpoint = socket;
  couple->state = State_Connected;
  couple->relatedUrls.push_back(url);
  _connections[machineId][url] = couple;
  couple->promise.setValue(socket);
}

/*
 * Corner case to manage (TODO):
 *
 * You are connecting to machineId foo, you are machineId bar. foo and bar are
 * on different sub-networks with the same netmask. They sadly got the same IP
 * on their subnet: 192.168.1.42. When trying to connect to foo from bar, we
 * will try to connect its endpoints, basically:
 *   - tcp://1.2.3.4:1333 (public IP)
 *   - tcp://192.168.1.42:1333 (subnet public IP)
 * If bar is listening on port 1333, we may connect to it instead of foo (our
 * real target).
 */
void TransportSocketCache::onSocketParallelConnectionAttempt(Future<void> fut,
                                                             MessageSocketPtr socket,
                                                             Url url,
                                                             const ServiceInfo& info)
{
  boost::mutex::scoped_lock lock(_socketMutex);

  if (_dying)
  {
    qiLogDebug() << "ConnectionAttempt: TransportSocketCache is closed";
    if (!fut.hasError())
    {
      _allPendingConnections.remove(socket);
      socket->disconnect();
    }
    return;
  }

  ConnectionMap::iterator machineIt = _connections.find(info.machineId());
  std::map<Url, ConnectionAttemptPtr>::iterator urlIt;
  if (machineIt != _connections.end())
    urlIt = machineIt->second.find(url);

  if (machineIt == _connections.end() || urlIt == machineIt->second.end())
  {
    // The socket was disconnected at some point, and we removed it from our map:
    // return early.
    _allPendingConnections.remove(socket);
    socket->disconnect();
    return;
  }

  ConnectionAttemptPtr attempt = urlIt->second;
  attempt->attemptCount--;
  if (attempt->state != State_Pending)
  {
    qiLogDebug() << "Already connected: reject socket " << socket.get() << " endpoint " << url.str();
    _allPendingConnections.remove(socket);
    socket->disconnect();
    checkClear(attempt, info.machineId());
    return;
  }
  if (fut.hasError())
  {
    // Failing to connect to some of the endpoint is expected.
    qiLogDebug() << "Could not connect to service #" << info.serviceId() << " through url " << url.str();
    _allPendingConnections.remove(socket);
    // It's a critical error if we've exhausted all available endpoints.
    if (attempt->attemptCount == 0)
    {
      std::stringstream err;
      err << "Could not connect to service #" << info.serviceId() << ": no endpoint replied.";
      qiLogError() << err.str();
      attempt->promise.setError(err.str());
      attempt->state = State_Error;
      checkClear(attempt, info.machineId());
    }
    return;
  }
  qi::SignalLink disconnectionTracking = socket->disconnected.connect(
        &TransportSocketCache::onSocketDisconnected, this, url, info)
      .setCallType(MetaCallType_Direct);
  attempt->state = State_Connected;
  attempt->endpoint = socket;
  attempt->promise.setValue(socket);
  attempt->disconnectionTracking = disconnectionTracking;
  qiLogDebug() << "Connected to service #" << info.serviceId() << " through url " << url.str() << " and socket "
               << socket.get();
}

void TransportSocketCache::checkClear(ConnectionAttemptPtr attempt, const std::string& machineId)
{
  if ((attempt->attemptCount <= 0 && attempt->state != State_Connected) || attempt->state == State_Error)
  {
    ConnectionMap::iterator machineIt = _connections.find(machineId);
    if (machineIt == _connections.end())
      return;
    for (UrlVector::const_iterator uit = attempt->relatedUrls.begin(), end = attempt->relatedUrls.end(); uit != end;
         ++uit)
      machineIt->second.erase(*uit);
    if (machineIt->second.size() == 0)
      _connections.erase(machineIt);
  }
}

void TransportSocketCache::onSocketDisconnected(Url url, const ServiceInfo& info)
{
  // remove from the available connections
  boost::mutex::scoped_lock lock(_socketMutex);

  ConnectionMap::iterator machineIt = _connections.find(info.machineId());
  if (machineIt == _connections.end())
    return;

  machineIt->second[url]->state = State_Error;
  checkClear(machineIt->second[url], info.machineId());
}
}
