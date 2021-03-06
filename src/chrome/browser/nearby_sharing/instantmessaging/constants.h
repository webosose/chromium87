// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NEARBY_SHARING_INSTANTMESSAGING_CONSTANTS_H_
#define CHROME_BROWSER_NEARBY_SHARING_INSTANTMESSAGING_CONSTANTS_H_

#if defined(NDEBUG)
const char kInstantMessagingReceiveMessageAPI[] =
    "https://tachyon-playground-autopush-grpc.sandbox.googleapis.com/v1/"
    "messages:receiveExpress";

const char kInstantMessagingSendMessageAPI[] =
    "https://tachyon-playground-autopush-grpc.sandbox.googleapis.com/v1/"
    "message:sendExpress";
#else
const char kInstantMessagingReceiveMessageAPI[] =
    "https://instantmessaging-pa.googleapis.com/v1/messages:receiveExpress";

const char kInstantMessagingSendMessageAPI[] =
    "https://instantmessaging-pa.googleapis.com/v1/message:sendExpress";
#endif  // defined(NDEBUG)

// Template for optional OAuth2 authorization HTTP header.
const char kAuthorizationHeaderFormat[] = "Authorization: Bearer %s";

// Timeout for network calls to instantmessaging servers.
const base::TimeDelta kNetworkTimeout = base::TimeDelta::FromMilliseconds(2500);

#endif  // CHROME_BROWSER_NEARBY_SHARING_INSTANTMESSAGING_CONSTANTS_H_
