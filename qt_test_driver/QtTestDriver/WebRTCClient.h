#ifndef WEBRTC_CLIENT_H
#define WEBRTC_CLIENT_H

/*
 * 跟Qt解耦，将本头文件放在Qt头文件前面，避免信号槽冲突  
 */

#include <functional>
#include <memory>
#include <string>

#include "api/peer_connection_interface.h"
#include "api/video/video_frame.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

class WebRTCClient : public webrtc::PeerConnectionObserver,
                     public webrtc::CreateSessionDescriptionObserver {
public:
    typedef std::function<void(const std::string& type, const std::string& sdp)> LocalSdpReadyHandler;
    typedef std::function<void(const std::string& sdpMid, int sdpMLineIndex, const std::string& candidate)> IceCandidateReadyHandler;
    typedef std::function<void(const webrtc::VideoFrame& frame)> RemoteFrameHandler;

    explicit WebRTCClient();
    ~WebRTCClient();

    bool init();
    void uninit();

    void createOffer();
    void createAnswer();
    void setRemoteDescription(const std::string& sdp, const std::string& type);
    void addIceCandidate(const std::string& sdpMid,
                         int sdpMLineIndex,
                         const std::string& candidate);

    // 回调注册函数
    void onLocalSdpReady(LocalSdpReadyHandler cb);
    void onIceCandidateReady(IceCandidateReadyHandler cb);
    void onRemoteFrame(RemoteFrameHandler cb);

private:
    // PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {};
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {};
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

    // CreateSessionDescriptionObserver
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    void OnFailure(webrtc::RTCError error) override;

    bool createPeerConnection();
    void DeletePeerConnection();
    void AddTracks();

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;

    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;

    
    // 回调函数
    LocalSdpReadyHandler on_local_sdp_;
    IceCandidateReadyHandler on_ice_candidate_;
    RemoteFrameHandler on_remote_frame_;
};

#endif  // WEBRTC_CLIENT_H
