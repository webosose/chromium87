// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_PEER_CONNECTION_CONTROLS_H_
#define REMOTING_PROTOCOL_PEER_CONNECTION_CONTROLS_H_

#include "base/optional.h"

namespace remoting {
namespace protocol {

// Interface for changing peer connection parameters after the connection is
// established.
class PeerConnectionControls {
 public:
  virtual ~PeerConnectionControls() = default;

  // Sets preferred min and max bitrates for the peer connection. nullopt means
  // no preference.
  virtual void SetPreferredBitrates(base::Optional<int> min_bitrate_bps,
                                    base::Optional<int> max_bitrate_bps) = 0;

  // Performs an ICE restart. This causes the host to initiate a new SDP
  // offer/answer exchange, and restarts the ICE gathering/connection sequence.
  // This can be used to re-establish a connection, without needing to
  // re-authenticate the user.
  virtual void RequestIceRestart() = 0;

  // Requests a new SDP offer/answer exchange, without restarting ICE. This can
  // be used to change SDP configuration (for example, switching to a different
  // codec), without needing a full reconnection.
  virtual void RequestSdpRestart() = 0;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_PEER_CONNECTION_CONTROLS_H_
