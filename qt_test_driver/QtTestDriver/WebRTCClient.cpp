#include "WebRTCClient.h"

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include <api/video/i420_buffer.h>
#include "examples/peerconnection/client/defaults.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/video_track_source.h"
#include "rtc_base/thread.h"
#include "test/vcm_capturer.h"

#include <QDebug>

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
    return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                     << error.message();
  }
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return rtc::make_ref_counted<CapturerTrackSource>(std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

WebRTCClient::WebRTCClient() {}

WebRTCClient::~WebRTCClient() {}

bool WebRTCClient::init() {
    network_thread_ = rtc::Thread::CreateWithSocketServer();
    network_thread_->SetName("network_thread", nullptr);
    network_thread_->Start();

    worker_thread_ = rtc::Thread::Create();
    worker_thread_->SetName("worker_thread", nullptr);
    worker_thread_->Start();

    signaling_thread_ = rtc::Thread::Create();
    signaling_thread_->SetName("signaling_thread", nullptr);
    signaling_thread_->Start();

    peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
        network_thread_.get(), worker_thread_.get(), 
        signaling_thread_.get(), nullptr /* default_adm */, 
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        std::make_unique<webrtc::VideoEncoderFactoryTemplate<
            webrtc::LibvpxVp8EncoderTemplateAdapter,
            webrtc::LibvpxVp9EncoderTemplateAdapter,
            webrtc::OpenH264EncoderTemplateAdapter,
            webrtc::LibaomAv1EncoderTemplateAdapter>>(),
        std::make_unique<webrtc::VideoDecoderFactoryTemplate<
            webrtc::LibvpxVp8DecoderTemplateAdapter,
            webrtc::LibvpxVp9DecoderTemplateAdapter,
            webrtc::OpenH264DecoderTemplateAdapter,
            webrtc::Dav1dDecoderTemplateAdapter>>(),
        nullptr /* audio_mixer */, nullptr /* audio_processing */);

    if (!peer_connection_factory_) {
      RTC_LOG(LS_INFO) << u8"PeerConnectionFactory 创建失败";
      return false;
    }

    if (!createPeerConnection()) {
      RTC_LOG(LS_INFO) << u8"PeerConnection 创建失败";
    }

    AddTracks();

    return peer_connection_ != nullptr;
}

void WebRTCClient::uninit() {
    DeletePeerConnection();
    if (peer_connection_factory_) {
      peer_connection_factory_ = nullptr;
      network_thread_->Stop();
      worker_thread_->Stop();
      signaling_thread_->Stop();
    }
}

bool WebRTCClient::createPeerConnection() {
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    // TODO: ICE 服务器配置
    //webrtc::PeerConnectionInterface::IceServer server;
    //server.uri = GetPeerConnectionString();
    //config.servers.push_back(server);

    webrtc::PeerConnectionDependencies pc_dependencies(this);
    auto error_or_peer_connection =
        peer_connection_factory_->CreatePeerConnectionOrError(
            config, std::move(pc_dependencies));
    if (error_or_peer_connection.ok()) {
      peer_connection_ = std::move(error_or_peer_connection.value());
    }
    return peer_connection_ != nullptr;
}

void WebRTCClient::DeletePeerConnection() {
    RTC_LOG(LS_INFO) << "[WebRTCClient] 停止并清理 PeerConnection";
    qDebug() << "[WebRTCClient] 停止并清理 PeerConnection";
    if (peer_connection_) {
        peer_connection_->Close();
        peer_connection_ = nullptr;
    }
}

void WebRTCClient::AddTracks() {
    if (!peer_connection_->GetSenders().empty()) {
      return;  // Already added tracks.
    }

    signaling_thread_->PostTask([this]() {
      rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
          peer_connection_factory_->CreateAudioTrack(
              kAudioLabel, peer_connection_factory_
                               ->CreateAudioSource(cricket::AudioOptions())
                               .get()));
      auto result_or_error =
          peer_connection_->AddTrack(audio_track, {kStreamId});
      if (!result_or_error.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                          << result_or_error.error().message();
      }

      rtc::scoped_refptr<CapturerTrackSource> video_device =
          CapturerTrackSource::Create();
      if (video_device) {
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
            peer_connection_factory_->CreateVideoTrack(video_device,
                                                       kVideoLabel));
        // main_wnd_->StartLocalRenderer(video_track_.get());

        result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
        if (!result_or_error.ok()) {
          RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                            << result_or_error.error().message();
        }
      } else {
        RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
      }
    });
}

void WebRTCClient::createOffer() {
    if (!peer_connection_)
      return;
    peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRTCClient::createAnswer() {
    if (!peer_connection_) {
      return;
    }
    peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRTCClient::setRemoteDescription(const std::string& sdp,
                                        const std::string& type) {
    webrtc::SdpType sdp_type =
        type == "offer" ? webrtc::SdpType::kOffer : webrtc::SdpType::kAnswer;
    webrtc::SdpParseError error;
    auto desc = webrtc::CreateSessionDescription(sdp_type, sdp, &error);
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create().get(), std::move(desc).release());
}

void WebRTCClient::addIceCandidate(const std::string& sdpMid,
                                   int sdpMLineIndex,
                                   const std::string& candidate) {
    webrtc::SdpParseError error;
    auto ice =
        webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, candidate, &error);
    if (ice)
      peer_connection_->AddIceCandidate(ice);
}

void WebRTCClient::onLocalSdpReady(LocalSdpReadyHandler cb) {
    on_local_sdp_ = cb;
}

void WebRTCClient::onIceCandidateReady(IceCandidateReadyHandler cb) {
    on_ice_candidate_ = cb;
}

void WebRTCClient::onRemoteFrame(RemoteFrameHandler cb) {
    on_remote_frame_ = std::move(cb);
}

// PeerConnectionObserver
void WebRTCClient::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  candidate->ToString(&sdp);

  if (on_ice_candidate_) {
    on_ice_candidate_(candidate->sdp_mid(), candidate->sdp_mline_index(), sdp);
  }
}

// CreateSessionDescriptionObserver
void WebRTCClient::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create().get(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  if (on_local_sdp_)
    on_local_sdp_(webrtc::SdpTypeToString(desc->GetType()), sdp);
}

void WebRTCClient::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_INFO) << u8"创建 offer/answer 失败:"
             << error.message();
}

void WebRTCClient::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  auto track = transceiver->receiver()->track();
  if (track && track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    RTC_LOG(LS_INFO) << u8"收到远端视频流";
    // 后续这里将 VideoFrame 转为 QImage，emit 给 Qt UI
  }
}