(function () {
  const remoteTracks = document.getElementById("remoteTracks");
  const remoteEmpty = document.getElementById("remoteEmpty");
  const localPreview = document.getElementById("localPreview");
  const localEmpty = document.getElementById("localEmpty");

  let room = null;
  let leavingLocally = false;
  let currentCallType = "voice";

  function emitHostEvent(payload) {
    try {
      document.title = "__memochat__:" + JSON.stringify(payload);
    } catch (error) {
      console.error(error);
    }
  }

  function setEmptyState(target, visible) {
    if (!target) {
      return;
    }
    target.style.display = visible ? "flex" : "none";
  }

  function refreshRemoteEmpty() {
    setEmptyState(remoteEmpty, !remoteTracks || remoteTracks.childElementCount === 0);
  }

  function refreshLocalEmpty() {
    setEmptyState(localEmpty, !localPreview || localPreview.childElementCount === 0);
  }

  function cleanupContainer(container) {
    if (!container) {
      return;
    }
    while (container.firstChild) {
      const node = container.firstChild;
      if (node.tagName === "VIDEO" || node.tagName === "AUDIO") {
        node.pause?.();
        node.srcObject = null;
      }
      container.removeChild(node);
    }
  }

  function cleanupRoomDom() {
    cleanupContainer(remoteTracks);
    cleanupContainer(localPreview);
    refreshRemoteEmpty();
    refreshLocalEmpty();
  }

  function trackElementFor(publication, track, isLocal) {
    if (!track) {
      return null;
    }
    const mediaElement = track.attach();
    if (track.kind === "video") {
      mediaElement.playsInline = true;
      mediaElement.autoplay = true;
      if (isLocal) {
        mediaElement.muted = true;
      }
    }
    mediaElement.dataset.trackSid = publication?.trackSid || "";
    return mediaElement;
  }

  function attachRemoteTrack(publication, track) {
    const mediaElement = trackElementFor(publication, track, false);
    if (!mediaElement) {
      return;
    }
    remoteTracks.appendChild(mediaElement);
    refreshRemoteEmpty();
    emitHostEvent({ type: "remote_track_ready" });
  }

  function attachLocalTrack(publication) {
    if (!publication || !publication.track || publication.kind !== "video") {
      return;
    }
    cleanupContainer(localPreview);
    const mediaElement = trackElementFor(publication, publication.track, true);
    if (!mediaElement) {
      return;
    }
    localPreview.appendChild(mediaElement);
    refreshLocalEmpty();
  }

  function detachTrack(track) {
    if (!track) {
      return;
    }
    const elements = track.detach ? track.detach() : [];
    elements.forEach((element) => element.remove?.());
    refreshRemoteEmpty();
    refreshLocalEmpty();
  }

  function parseMetadata(metadataJson) {
    try {
      return metadataJson ? JSON.parse(metadataJson) : {};
    } catch (error) {
      emitHostEvent({ type: "log", message: `metadata parse failed: ${error}` });
      return {};
    }
  }

  async function cleanupRoom() {
    if (!room) {
      cleanupRoomDom();
      return;
    }
    try {
      room.disconnect();
    } catch (error) {
      emitHostEvent({ type: "log", message: `disconnect failed: ${error}` });
    }
    room = null;
    cleanupRoomDom();
  }

  async function applyInitialTracks(metadata) {
    if (!room) {
      return;
    }
    const startMicEnabled = metadata.muted !== true;
    const startCameraEnabled = metadata.callType === "video" && metadata.cameraEnabled !== false;
    currentCallType = metadata.callType || "voice";

    await room.localParticipant.setMicrophoneEnabled(startMicEnabled);
    if (currentCallType === "video") {
      await room.localParticipant.setCameraEnabled(startCameraEnabled);
    } else {
      await room.localParticipant.setCameraEnabled(false);
    }

    room.localParticipant.videoTrackPublications.forEach((publication) => {
      attachLocalTrack(publication);
    });
  }

  function bindRoomEvents(nextRoom) {
    const { RoomEvent } = window.LivekitClient;

    nextRoom.on(RoomEvent.Connected, () => {
      emitHostEvent({ type: "joined" });
    });

    nextRoom.on(RoomEvent.Reconnecting, () => {
      emitHostEvent({ type: "reconnecting", message: "网络波动，正在重连" });
    });

    nextRoom.on(RoomEvent.Reconnected, () => {
      emitHostEvent({ type: "reconnecting", message: "网络已恢复" });
    });

    nextRoom.on(RoomEvent.Disconnected, (reason) => {
      const reasonText = leavingLocally ? "local_leave" : String(reason || "");
      leavingLocally = false;
      cleanupRoomDom();
      room = null;
      emitHostEvent({ type: "disconnected", reason: reasonText, recoverable: false });
    });

    nextRoom.on(RoomEvent.MediaDevicesError, (error) => {
      emitHostEvent({ type: "media_error", message: String(error?.message || error || "媒体设备异常") });
    });

    nextRoom.on(RoomEvent.LocalTrackPublished, (publication) => {
      attachLocalTrack(publication);
    });

    nextRoom.on(RoomEvent.LocalTrackUnpublished, (publication) => {
      detachTrack(publication?.track);
    });

    nextRoom.on(RoomEvent.TrackSubscribed, (track, publication) => {
      attachRemoteTrack(publication, track);
    });

    nextRoom.on(RoomEvent.TrackUnsubscribed, (track) => {
      detachTrack(track);
    });

    nextRoom.on(RoomEvent.ParticipantDisconnected, () => {
      refreshRemoteEmpty();
    });
  }

  async function joinRoom(wsUrl, token, metadataJson) {
    if (!window.LivekitClient || !window.LivekitClient.Room) {
      emitHostEvent({ type: "media_error", message: "LiveKit 前端依赖未加载" });
      return;
    }

    const metadata = parseMetadata(metadataJson);
    leavingLocally = false;
    await cleanupRoom();

    const nextRoom = new window.LivekitClient.Room({
      adaptiveStream: true,
      dynacast: true,
      stopLocalTrackOnUnpublish: false
    });
    bindRoomEvents(nextRoom);
    room = nextRoom;

    try {
      await nextRoom.connect(wsUrl, token, { autoSubscribe: true });
      await applyInitialTracks(metadata);
    } catch (error) {
      const message = String(error?.message || error || "连接 LiveKit 失败");
      if (/permission|denied|notallowed/i.test(message)) {
        emitHostEvent({
          type: "permission_error",
          deviceType: metadata.callType === "video" ? "camera" : "microphone",
          message
        });
      } else {
        emitHostEvent({ type: "media_error", message });
      }
      await cleanupRoom();
    }
  }

  async function toggleMic() {
    if (!room) {
      return;
    }
    const participant = room.localParticipant;
    const enabled = participant.isMicrophoneEnabled !== true;
    await participant.setMicrophoneEnabled(enabled);
  }

  async function toggleCamera() {
    if (!room || currentCallType !== "video") {
      return;
    }
    const participant = room.localParticipant;
    const enabled = participant.isCameraEnabled !== true;
    await participant.setCameraEnabled(enabled);
    if (!enabled) {
      cleanupContainer(localPreview);
      refreshLocalEmpty();
    }
  }

  async function leaveRoom() {
    leavingLocally = true;
    await cleanupRoom();
    emitHostEvent({ type: "disconnected", reason: "local_leave", recoverable: false });
  }

  window.addEventListener("error", (event) => {
    emitHostEvent({ type: "media_error", message: String(event.error?.message || event.message || "页面脚本异常") });
  });

  refreshRemoteEmpty();
  refreshLocalEmpty();

  window.MemoChatLiveKit = {
    join(payload) {
      const nextPayload = payload || {};
      return joinRoom(nextPayload.wsUrl || "", nextPayload.token || "", JSON.stringify(nextPayload.metadata || {}));
    },
    toggleMic() {
      return toggleMic();
    },
    toggleCamera() {
      return toggleCamera();
    },
    leave() {
      return leaveRoom();
    }
  };
})();
