/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/p2p/base/stunport.h"

#include "webrtc/p2p/base/common.h"
#include "webrtc/p2p/base/portallocator.h"
#include "webrtc/p2p/base/stun.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/common.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/ipaddress.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/nethelpers.h"
#include "third_party/boringssl/src/include/openssl/aes.h" //jay add
#include <string.h>

namespace cricket {

// TODO: Move these to a common place (used in relayport too)
const int KEEPALIVE_DELAY = 10 * 1000;  // 10 seconds - sort timeouts
const int RETRY_TIMEOUT = 50 * 1000;    // ICE says 50 secs
// Stop sending STUN binding requests after this amount of time
// (in milliseconds) because the connection binding requests should keep
// the NAT binding alive.
const int KEEP_ALIVE_TIMEOUT = 2 * 60 * 1000;  // 2 minutes

    
    
    //jay add
    
    static std::string Connection_EncodeAES( const std::string& password, const std::string& data,bool bCBC ,const std::string ivkey)
    {
        AES_KEY aes_key;
        if(AES_set_encrypt_key((const unsigned char*)password.c_str(), password.length() * 8, &aes_key) < 0)
        {
            assert(false);
            return "";
        }
        std::string strRet;
        std::string data_bak = data;
        unsigned int data_length = data_bak.length();
        int padding = 0;
        if (data_bak.length() % AES_BLOCK_SIZE > 0)
        {
            padding =  AES_BLOCK_SIZE - data_bak.length() % AES_BLOCK_SIZE;
        }
        data_length += padding;
        while (padding > 0)
        {
            data_bak += '\0';
            padding--;
        }
        unsigned char iv[AES_BLOCK_SIZE+1];//加密的初始化向量
        for(int i=0; i<AES_BLOCK_SIZE; ++i)//iv一般设置为全0,可以设置其他，但是加密解密要一样就行
            iv[i]=0;
        if(ivkey.empty()){
            strcpy((char*)&iv[0],"0102030405060708");
        }
        else
        {
            strncpy((char*)&iv[0],ivkey.c_str(),AES_BLOCK_SIZE);
        }
        for(unsigned int i = 0; i < data_length/AES_BLOCK_SIZE; i++)
        {
            std::string str16 = data_bak.substr(i*AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            unsigned char out[AES_BLOCK_SIZE];
            ::memset(out, 0, AES_BLOCK_SIZE);
            if(false==bCBC){
                AES_encrypt((const unsigned char*)str16.c_str(), out, &aes_key);
            }
            else{
                int len=str16.size();
                AES_cbc_encrypt((unsigned char*)str16.c_str(), (unsigned char*)out, len, &aes_key, iv, AES_ENCRYPT);
            }
            //AES_ecb_encrypt((const unsigned char*)str16.c_str(), out, &aes_key,AES_ENCRYPT);
            strRet += std::string((const char*)out, AES_BLOCK_SIZE);
        }
        return strRet;
        
    }
    
    static std::string Connection_DecodeAES( const std::string& strPassword, const std::string& strData ,bool bCBC,const std::string ivkey)
    {
        if(strPassword.size() != 16 && strPassword.size() != 32 && strPassword.size() != 24)
        {
            return "";
        }
        AES_KEY aes_key;
        if(AES_set_decrypt_key((const unsigned char*)strPassword.c_str(), strPassword.length() * 8, &aes_key) < 0)
        {
            assert(false);
            return "";
        }
        unsigned char iv[AES_BLOCK_SIZE];//加密的初始化向量
        for(int i=0; i<AES_BLOCK_SIZE; ++i)//iv一般设置为全0,可以设置其他，但是加密解密要一样就行
            iv[i]=0;
        //strcpy((char*)&iv[0],"0102030405060708");
        if(ivkey.empty()){
            strcpy((char*)&iv[0],"0102030405060708");
        }
        else
        {
            strncpy((char*)&iv[0],ivkey.c_str(),AES_BLOCK_SIZE);
        }
        
        std::string strRet;
        for(unsigned int i = 0; i < strData.length()/AES_BLOCK_SIZE; i++)
        {
            std::string str16 = strData.substr(i*AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            unsigned char out[AES_BLOCK_SIZE];
            ::memset(out, 0, AES_BLOCK_SIZE);
            if(false==bCBC)
            {
                AES_decrypt((const unsigned char*)str16.c_str(), out, &aes_key);
            }
            else
            {
                int len=str16.size();
                AES_cbc_encrypt((unsigned char*)str16.c_str(), (unsigned char*)out, len, &aes_key, iv, AES_DECRYPT);
            }
            strRet += std::string((const char*)out, AES_BLOCK_SIZE);
        }
        return strRet;
        
    }
    
    
    
    
    
    
// Handles a binding request sent to the STUN server.
class StunBindingRequest : public StunRequest {
 public:
  StunBindingRequest(UDPPort* port,
                     const rtc::SocketAddress& addr,
                     uint32_t deadline)
      : port_(port), server_addr_(addr), deadline_(deadline) {
    start_time_ = rtc::Time();
  }

  virtual ~StunBindingRequest() {
  }

  const rtc::SocketAddress& server_addr() const { return server_addr_; }

  virtual void Prepare(StunMessage* request) override {
    request->SetType(STUN_BINDING_REQUEST);
  }

  virtual void OnResponse(StunMessage* response) override {
    const StunAddressAttribute* addr_attr =
        response->GetAddress(STUN_ATTR_MAPPED_ADDRESS);
    if (!addr_attr) {
      LOG(LS_ERROR) << "Binding response missing mapped address.";
    } else if (addr_attr->family() != STUN_ADDRESS_IPV4 &&
               addr_attr->family() != STUN_ADDRESS_IPV6) {
      LOG(LS_ERROR) << "Binding address has bad family";
    } else {
      rtc::SocketAddress addr(addr_attr->ipaddr(), addr_attr->port());
      port_->OnStunBindingRequestSucceeded(server_addr_, addr);
    }

    // We will do a keep-alive regardless of whether this request succeeds.
    // It will be stopped after |deadline_| mostly to conserve the battery life.
    if (rtc::Time() <= deadline_) {
      port_->requests_.SendDelayed(
          new StunBindingRequest(port_, server_addr_, deadline_),
          port_->stun_keepalive_delay());
    }
  }

  virtual void OnErrorResponse(StunMessage* response) override {
    const StunErrorCodeAttribute* attr = response->GetErrorCode();
    if (!attr) {
      LOG(LS_ERROR) << "Bad allocate response error code";
    } else {
      LOG(LS_ERROR) << "Binding error response:"
                 << " class=" << attr->eclass()
                 << " number=" << attr->number()
                 << " reason='" << attr->reason() << "'";
    }

    port_->OnStunBindingOrResolveRequestFailed(server_addr_);

    uint32_t now = rtc::Time();
    if (now <= deadline_ && rtc::TimeDiff(now, start_time_) <= RETRY_TIMEOUT) {
      port_->requests_.SendDelayed(
          new StunBindingRequest(port_, server_addr_, deadline_),
          port_->stun_keepalive_delay());
    }
  }

  virtual void OnTimeout() override {
    LOG(LS_ERROR) << "Binding request timed out from "
      << port_->GetLocalAddress().ToSensitiveString()
      << " (" << port_->Network()->name() << ")";

    port_->OnStunBindingOrResolveRequestFailed(server_addr_);
  }

 private:
  UDPPort* port_;
  const rtc::SocketAddress server_addr_;
  uint32_t start_time_;
  uint32_t deadline_;
};

UDPPort::AddressResolver::AddressResolver(
    rtc::PacketSocketFactory* factory)
    : socket_factory_(factory) {}

UDPPort::AddressResolver::~AddressResolver() {
  for (ResolverMap::iterator it = resolvers_.begin();
       it != resolvers_.end(); ++it) {
    // TODO(guoweis): Change to asynchronous DNS resolution to prevent the hang
    // when passing true to the Destroy() which is a safer way to avoid the code
    // unloaded before the thread exits. Please see webrtc bug 5139.
    it->second->Destroy(false);
  }
}

void UDPPort::AddressResolver::Resolve(
    const rtc::SocketAddress& address) {
  if (resolvers_.find(address) != resolvers_.end())
    return;

  rtc::AsyncResolverInterface* resolver =
      socket_factory_->CreateAsyncResolver();
  resolvers_.insert(
      std::pair<rtc::SocketAddress, rtc::AsyncResolverInterface*>(
          address, resolver));

  resolver->SignalDone.connect(this,
                               &UDPPort::AddressResolver::OnResolveResult);

  resolver->Start(address);
}

bool UDPPort::AddressResolver::GetResolvedAddress(
    const rtc::SocketAddress& input,
    int family,
    rtc::SocketAddress* output) const {
  ResolverMap::const_iterator it = resolvers_.find(input);
  if (it == resolvers_.end())
    return false;

  return it->second->GetResolvedAddress(family, output);
}

void UDPPort::AddressResolver::OnResolveResult(
    rtc::AsyncResolverInterface* resolver) {
  for (ResolverMap::iterator it = resolvers_.begin();
       it != resolvers_.end(); ++it) {
    if (it->second == resolver) {
      SignalDone(it->first, resolver->GetError());
      return;
    }
  }
}

UDPPort::UDPPort(rtc::Thread* thread,
                 rtc::PacketSocketFactory* factory,
                 rtc::Network* network,
                 rtc::AsyncPacketSocket* socket,
                 const std::string& username,
                 const std::string& password,
                 const std::string& origin,
                 bool emit_local_for_anyaddress)
    : Port(thread,
           factory,
           network,
           socket->GetLocalAddress().ipaddr(),
           username,
           password),
      requests_(thread),
      socket_(socket),
      error_(0),
      ready_(false),
      stun_keepalive_delay_(KEEPALIVE_DELAY),
      emit_local_for_anyaddress_(emit_local_for_anyaddress) {
  requests_.set_origin(origin);
}

UDPPort::UDPPort(rtc::Thread* thread,
                 rtc::PacketSocketFactory* factory,
                 rtc::Network* network,
                 const rtc::IPAddress& ip,
                 uint16_t min_port,
                 uint16_t max_port,
                 const std::string& username,
                 const std::string& password,
                 const std::string& origin,
                 bool emit_local_for_anyaddress)
    : Port(thread,
           LOCAL_PORT_TYPE,
           factory,
           network,
           ip,
           min_port,
           max_port,
           username,
           password),
      requests_(thread),
      socket_(NULL),
      error_(0),
      ready_(false),
      stun_keepalive_delay_(KEEPALIVE_DELAY),
      emit_local_for_anyaddress_(emit_local_for_anyaddress) {
  requests_.set_origin(origin);
}

bool UDPPort::Init() {
  if (!SharedSocket()) {
    ASSERT(socket_ == NULL);
    socket_ = socket_factory()->CreateUdpSocket(
        rtc::SocketAddress(ip(), 0), min_port(), max_port());
    if (!socket_) {
      LOG_J(LS_WARNING, this) << "UDP socket creation failed";
      return false;
    }
    socket_->SignalReadPacket.connect(this, &UDPPort::OnReadPacket);
  }
  socket_->SignalSentPacket.connect(this, &UDPPort::OnSentPacket);
  socket_->SignalReadyToSend.connect(this, &UDPPort::OnReadyToSend);
  socket_->SignalAddressReady.connect(this, &UDPPort::OnLocalAddressReady);
  requests_.SignalSendPacket.connect(this, &UDPPort::OnSendPacket);
  return true;
}

UDPPort::~UDPPort() {
  if (!SharedSocket())
    delete socket_;
}

void UDPPort::PrepareAddress() {
  ASSERT(requests_.empty());
  if (socket_->GetState() == rtc::AsyncPacketSocket::STATE_BOUND) {
    OnLocalAddressReady(socket_, socket_->GetLocalAddress());
  }
}

void UDPPort::MaybePrepareStunCandidate() {
  // Sending binding request to the STUN server if address is available to
  // prepare STUN candidate.
  if (!server_addresses_.empty()) {
    SendStunBindingRequests();
  } else {
    // Port is done allocating candidates.
    MaybeSetPortCompleteOrError();
  }
}

Connection* UDPPort::CreateConnection(const Candidate& address,
                                      CandidateOrigin origin) {
  if (!SupportsProtocol(address.protocol())) {
    return NULL;
  }

  if (!IsCompatibleAddress(address.address())) {
    return NULL;
  }

  if (SharedSocket() && Candidates()[0].type() != LOCAL_PORT_TYPE) {
    ASSERT(false);
    return NULL;
  }

  Connection* conn = new ProxyConnection(this, 0, address);
  AddConnection(conn);
  return conn;
}
    std::string UDPPort::rtpAesKey_ ="";//"12345678901234567890123456789012"; //jay add
    
    void UDPPort::setChannelRtpEncryptKey(const std::string& aesKey)
    {
        rtpAesKey_=aesKey;
    }
    
    
int UDPPort::SendTo(const void* data, size_t size,
                    const rtc::SocketAddress& addr,
                    const rtc::PacketOptions& options,
                    bool payload) {
    
    std::string encryptRtpData;
    //jay add to encrypt rtp header
    int packetSize=size;
    if( rtpAesKey_.empty()==false){
        std::string tmpRawData;
        //        time_t nowt=time(0);
        static uint32_t s_sendSeq=rand();
        s_sendSeq++;
        tmpRawData.append((const char*)&s_sendSeq,4);
        uint32_t nTmpSize = htonl(packetSize);
        tmpRawData.append((const char*)&nTmpSize,4);
        tmpRawData.append((char*)data,size);
        encryptRtpData=Connection_EncodeAES( rtpAesKey_, tmpRawData,true,"0102030405060708");
        int appendSize=rand()%7;
        for(int kkk=0;kkk<appendSize;kkk++){
            uint8_t ccc=rand()%254;
            encryptRtpData.push_back(ccc);
        }
        data=encryptRtpData.data();
        size=encryptRtpData.size();
        
    }
    //end.
 
    
    
    
    
  int sent = socket_->SendTo(data, size, addr, options);
  if (sent < 0) {
    error_ = socket_->GetError();
    LOG_J(LS_ERROR, this) << "UDP send of " << size
                          << " bytes failed with error " << error_;
  }
  return sent;
}

int UDPPort::SetOption(rtc::Socket::Option opt, int value) {
  return socket_->SetOption(opt, value);
}

int UDPPort::GetOption(rtc::Socket::Option opt, int* value) {
  return socket_->GetOption(opt, value);
}

int UDPPort::GetError() {
  return error_;
}

void UDPPort::OnLocalAddressReady(rtc::AsyncPacketSocket* socket,
                                  const rtc::SocketAddress& address) {
  // When adapter enumeration is disabled and binding to the any address, the
  // default local address will be issued as a candidate instead if
  // |emit_local_for_anyaddress| is true. This is to allow connectivity for
  // applications which absolutely requires a HOST candidate.
  rtc::SocketAddress addr = address;

  // If MaybeSetDefaultLocalAddress fails, we keep the "any" IP so that at
  // least the port is listening.
  MaybeSetDefaultLocalAddress(&addr);

  AddAddress(addr, addr, rtc::SocketAddress(), UDP_PROTOCOL_NAME, "", "",
             LOCAL_PORT_TYPE, ICE_TYPE_PREFERENCE_HOST, 0, false);
  MaybePrepareStunCandidate();
}

void UDPPort::OnReadPacket(rtc::AsyncPacketSocket* socket,
                           const char* data,
                           size_t size,
                           const rtc::SocketAddress& remote_addr,
                           const rtc::PacketTime& packet_time) {
  ASSERT(socket == socket_);
  ASSERT(!remote_addr.IsUnresolvedIP());

  // Look for a response from the STUN server.
  // Even if the response doesn't match one of our outstanding requests, we
  // will eat it because it might be a response to a retransmitted packet, and
  // we already cleared the request when we got the first response.
  if (server_addresses_.find(remote_addr) != server_addresses_.end()) {
    requests_.CheckResponse(data, size);
    return;
  }

    
    //jay add to decrypted
    std::string stunData;
    if(rtpAesKey_.empty()==false){
        if(size>=16){
            int dataLen=size;
            if(size %16){
                dataLen -=size%16;
            }
            std::string tmpData(data,dataLen);
            std::string decyptData=Connection_DecodeAES( rtpAesKey_, tmpData ,true,"0102030405060708");
            //(decyptData.size());
            uint32_t nsize=0;
            memcpy((char*)&nsize,decyptData.data()+4,4);
            //            uint32_t nTmp = htonl(lVal);
            nsize = ntohl(nsize);
            uint32_t leftLen=decyptData.size()-4-4;
            if(nsize>leftLen){
                return;
            }
            stunData.append(decyptData.data()+8,nsize);
            data=stunData.data();
            size=nsize;
        }else{
            //TODO, log
        }
    }
    
    //end.

    
    
    
    
  if (Connection* conn = GetConnection(remote_addr)) {
    conn->OnReadPacket(data, size, packet_time);
  } else {
    Port::OnReadPacket(data, size, remote_addr, PROTO_UDP);
  }
}

void UDPPort::OnSentPacket(rtc::AsyncPacketSocket* socket,
                           const rtc::SentPacket& sent_packet) {
  PortInterface::SignalSentPacket(sent_packet);
}

void UDPPort::OnReadyToSend(rtc::AsyncPacketSocket* socket) {
  Port::OnReadyToSend();
}

void UDPPort::SendStunBindingRequests() {
  // We will keep pinging the stun server to make sure our NAT pin-hole stays
  // open until the deadline (specified in SendStunBindingRequest).
  ASSERT(requests_.empty());

  for (ServerAddresses::const_iterator it = server_addresses_.begin();
       it != server_addresses_.end(); ++it) {
    SendStunBindingRequest(*it);
  }
}

void UDPPort::ResolveStunAddress(const rtc::SocketAddress& stun_addr) {
  if (!resolver_) {
    resolver_.reset(new AddressResolver(socket_factory()));
    resolver_->SignalDone.connect(this, &UDPPort::OnResolveResult);
  }

  LOG_J(LS_INFO, this) << "Starting STUN host lookup for "
                       << stun_addr.ToSensitiveString();
  resolver_->Resolve(stun_addr);
}

void UDPPort::OnResolveResult(const rtc::SocketAddress& input,
                              int error) {
  ASSERT(resolver_.get() != NULL);

  rtc::SocketAddress resolved;
  if (error != 0 ||
      !resolver_->GetResolvedAddress(input, ip().family(), &resolved))  {
    LOG_J(LS_WARNING, this) << "StunPort: stun host lookup received error "
                            << error;
    OnStunBindingOrResolveRequestFailed(input);
    return;
  }

  server_addresses_.erase(input);

  if (server_addresses_.find(resolved) == server_addresses_.end()) {
    server_addresses_.insert(resolved);
    SendStunBindingRequest(resolved);
  }
}

void UDPPort::SendStunBindingRequest(const rtc::SocketAddress& stun_addr) {
  if (stun_addr.IsUnresolvedIP()) {
    ResolveStunAddress(stun_addr);

  } else if (socket_->GetState() == rtc::AsyncPacketSocket::STATE_BOUND) {
    // Check if |server_addr_| is compatible with the port's ip.
    if (IsCompatibleAddress(stun_addr)) {
      requests_.Send(new StunBindingRequest(this, stun_addr,
                                            rtc::Time() + KEEP_ALIVE_TIMEOUT));
    } else {
      // Since we can't send stun messages to the server, we should mark this
      // port ready.
      LOG(LS_WARNING) << "STUN server address is incompatible.";
      OnStunBindingOrResolveRequestFailed(stun_addr);
    }
  }
}

bool UDPPort::MaybeSetDefaultLocalAddress(rtc::SocketAddress* addr) const {
  if (!addr->IsAnyIP() || !emit_local_for_anyaddress_ ||
      !Network()->default_local_address_provider()) {
    return true;
  }
  rtc::IPAddress default_address;
  bool result =
      Network()->default_local_address_provider()->GetDefaultLocalAddress(
          addr->family(), &default_address);
  if (!result || default_address.IsNil()) {
    return false;
  }

  addr->SetIP(default_address);
  return true;
}

void UDPPort::OnStunBindingRequestSucceeded(
    const rtc::SocketAddress& stun_server_addr,
    const rtc::SocketAddress& stun_reflected_addr) {
  if (bind_request_succeeded_servers_.find(stun_server_addr) !=
          bind_request_succeeded_servers_.end()) {
    return;
  }
  bind_request_succeeded_servers_.insert(stun_server_addr);

  // If socket is shared and |stun_reflected_addr| is equal to local socket
  // address, or if the same address has been added by another STUN server,
  // then discarding the stun address.
  // For STUN, related address is the local socket address.
  if ((!SharedSocket() || stun_reflected_addr != socket_->GetLocalAddress()) &&
      !HasCandidateWithAddress(stun_reflected_addr)) {

    rtc::SocketAddress related_address = socket_->GetLocalAddress();
    // If we can't stamp the related address correctly, empty it to avoid leak.
    if (!MaybeSetDefaultLocalAddress(&related_address) ||
        !(candidate_filter() & CF_HOST)) {
      // If candidate filter doesn't have CF_HOST specified, empty raddr to
      // avoid local address leakage.
      related_address = rtc::EmptySocketAddressWithFamily(
          related_address.family());
    }

    AddAddress(stun_reflected_addr, socket_->GetLocalAddress(), related_address,
               UDP_PROTOCOL_NAME, "", "", STUN_PORT_TYPE,
               ICE_TYPE_PREFERENCE_SRFLX, 0, false);
  }
  MaybeSetPortCompleteOrError();
}

void UDPPort::OnStunBindingOrResolveRequestFailed(
    const rtc::SocketAddress& stun_server_addr) {
  if (bind_request_failed_servers_.find(stun_server_addr) !=
          bind_request_failed_servers_.end()) {
    return;
  }
  bind_request_failed_servers_.insert(stun_server_addr);
  MaybeSetPortCompleteOrError();
}

void UDPPort::MaybeSetPortCompleteOrError() {
  if (ready_)
    return;

  // Do not set port ready if we are still waiting for bind responses.
  const size_t servers_done_bind_request = bind_request_failed_servers_.size() +
      bind_request_succeeded_servers_.size();
  if (server_addresses_.size() != servers_done_bind_request) {
    return;
  }

  // Setting ready status.
  ready_ = true;

  // The port is "completed" if there is no stun server provided, or the bind
  // request succeeded for any stun server, or the socket is shared.
  if (server_addresses_.empty() ||
      bind_request_succeeded_servers_.size() > 0 ||
      SharedSocket()) {
    SignalPortComplete(this);
  } else {
    SignalPortError(this);
  }
}

// TODO: merge this with SendTo above.
void UDPPort::OnSendPacket(const void* data, size_t size, StunRequest* req) {
  StunBindingRequest* sreq = static_cast<StunBindingRequest*>(req);
  rtc::PacketOptions options(DefaultDscpValue());
  if (socket_->SendTo(data, size, sreq->server_addr(), options) < 0)
    PLOG(LERROR, socket_->GetError()) << "sendto";
}

bool UDPPort::HasCandidateWithAddress(const rtc::SocketAddress& addr) const {
  const std::vector<Candidate>& existing_candidates = Candidates();
  std::vector<Candidate>::const_iterator it = existing_candidates.begin();
  for (; it != existing_candidates.end(); ++it) {
    if (it->address() == addr)
      return true;
  }
  return false;
}

}  // namespace cricket
