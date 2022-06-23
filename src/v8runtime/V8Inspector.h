/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <condition_variable>
#include <cxxreact/MessageQueueThread.h>
#include "jsinspector/InspectorInterfaces.h"
#include "v8-inspector.h"

namespace rnv8 {

class InspectorClient;
class ClientConnectionWrapper;

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
 public:
  explicit InspectorFrontend(
      InspectorClient *client,
      v8::Local<v8::Context> context);
  ~InspectorFrontend() override = default;

 private:
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override {}

 private:
  InspectorClient *client_;
  v8::Isolate *isolate_;
  v8::Global<v8::Context> context_;
};

class InspectorClient final
    : public v8_inspector::V8InspectorClient,
      public std::enable_shared_from_this<InspectorClient> {
 public:
  InspectorClient(
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
      v8::Local<v8::Context> context,
      const std::string &appName,
      const std::string &deviceName);
  ~InspectorClient() override;

  void runMessageLoopOnPause(int contextGroupId) override;
  void quitMessageLoopOnPause() override;
  v8::Local<v8::Context> ensureDefaultContextInGroup(int group_id) override;

  void ConnectToReactFrontend();
  void Disconnect();
  void DisconnectFromReactFrontend();
  void SendRemoteMessage(const v8_inspector::StringView &message);
  bool IsPaused();
  void AwakePauseLockWithMessage(const std::string &message);
  void DispatchProxy(const std::string &message);
  void DispatchProtocolMessage(const std::string &message);
  void DispatchProtocolMessages(const std::vector<std::string> &messages);

  v8::Isolate *GetIsolate();
  v8::Global<v8::Context> &GetContext();
  v8_inspector::V8InspectorSession *GetInspectorSession();
  std::shared_ptr<facebook::react::MessageQueueThread> jsQueue_;

 private:
  static std::string CreateInspectorName(
      const std::string &appName,
      const std::string &deviceName);

  std::weak_ptr<InspectorClient> CreateWeakPtr();

 private:
  static int nextContextGroupId_;

  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
  std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
  v8::Isolate *isolate_;
  v8::Global<v8::Context> context_;

  std::unique_ptr<facebook::react::IRemoteConnection> remoteConn_;
  std::string inspectorName_;

  std::mutex connectionMutex_;
  std::mutex pauseMutex_;
  std::condition_variable pauseWaitable_;
  bool paused_ = false;
  std::vector<std::string> protocolMessageQueue_;

  int pageId_;
  std::shared_ptr<ClientConnectionWrapper> clientConnectionWrapper_;
};

} // namespace rnv8
