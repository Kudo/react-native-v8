#include "V8Inspector.h"

namespace facebook {

static const char kInspectorName[] = "React Native V8 Inspector";
static const int kContextGroupId = 1;

namespace {
std::string ToSTLString(
    v8::Isolate *isolate,
    const v8_inspector::StringView &stringView) {
  v8::HandleScope scopedIsolate(isolate);
  int length = static_cast<int>(stringView.length());
  v8::Local<v8::String> v8String =
      (stringView.is8Bit()
           ? v8::String::NewFromOneByte(
                 isolate,
                 reinterpret_cast<const uint8_t *>(stringView.characters8()),
                 v8::NewStringType::kNormal,
                 length)
           : v8::String::NewFromTwoByte(
                 isolate,
                 reinterpret_cast<const uint16_t *>(stringView.characters16()),
                 v8::NewStringType::kNormal,
                 length))
          .ToLocalChecked();
  v8::String::Utf8Value utf8V8String(isolate, v8String);
  if (*utf8V8String)
    return std::string(*utf8V8String, utf8V8String.length());
  else
    return std::string();
}

} // namespace

InspectorFrontend::InspectorFrontend(
    InspectorClient *client,
    v8::Local<v8::Context> context)
    : client_(client) {
  isolate_ = context->GetIsolate();
}

void InspectorFrontend::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

void InspectorFrontend::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

class LocalConnection : public react::ILocalConnection {
 public:
  LocalConnection(InspectorClient &client) : client_(client) {}
  ~LocalConnection() override {}

  void sendMessage(std::string message) override {
    v8::HandleScope scopedIsolate(client_.isolate_);
    v8_inspector::StringView messageView(
        reinterpret_cast<const uint8_t *>(message.data()), message.size());
    {
      v8::SealHandleScope seal_scopedIsolate(client_.isolate_);
      client_.session_->dispatchProtocolMessage(messageView);
    }
  }

  void disconnect() override {}

 private:
  InspectorClient &client_;
};

InspectorClient::InspectorClient(v8::Local<v8::Context> context) {
  isolate_ = context->GetIsolate();
  channel_.reset(new InspectorFrontend(this, context));
  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  v8_inspector::StringView inspectorName(
      reinterpret_cast<const uint8_t *>(kInspectorName),
      sizeof(kInspectorName));

  session_ =
      inspector_->connect(kContextGroupId, channel_.get(), inspectorName);
  // context->SetAlignedPointerInEmbedderData(kInspectorClientIndex, this);
  inspector_->contextCreated(
      v8_inspector::V8ContextInfo(context, kContextGroupId, inspectorName));

  context_.Reset(isolate_, context);

  react::getInspectorInstance().addPage(
      kInspectorName,
      "V8",
      [this](std::unique_ptr<react::IRemoteConnection> remoteConn) {
        this->remoteConn_ = std::move(remoteConn);
        return std::make_unique<LocalConnection>(*this);
      });
}

InspectorClient::~InspectorClient() {}

void InspectorClient::runMessageLoopOnPause(int contextGroupId) {
  paused_ = true;
  std::unique_lock<std::mutex> lock(pauseMutex_);
  pauseWaitable_.wait(lock, [this] { return !paused_; });
}

void InspectorClient::quitMessageLoopOnPause() {
  std::lock_guard<std::mutex> lock(pauseMutex_);
  paused_ = false;
  pauseWaitable_.notify_all();
}

v8::Local<v8::Context> InspectorClient::ensureDefaultContextInGroup(
    int group_id) {
  return context_.Get(isolate_);
}

void InspectorClient::SendRemoteMessage(
    const v8_inspector::StringView &message) {
  if (remoteConn_) {
    std::string messageString = ToSTLString(isolate_, message);
    remoteConn_->onMessage(messageString);
  }
}

} // namespace facebook
