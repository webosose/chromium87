// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab;

import java.nio.ByteBuffer;

/** Contains the state for a WebContents. */
public class WebContentsState {
    /**
     * Version number of the format used to save the WebContents navigation history, as returned by
     * TabStateJni.get().getContentsStateAsByteBuffer(). Version labels:
     *   0 - Chrome m18
     *   1 - Chrome m25
     *   2 - Chrome m26+
     */
    public static final int CONTENTS_STATE_CURRENT_VERSION = 2;

    private final ByteBuffer mBuffer;
    private int mVersion;

    public WebContentsState(ByteBuffer buffer) {
        mBuffer = buffer;
    }

    public ByteBuffer buffer() {
        return mBuffer;
    }

    public int version() {
        return mVersion;
    }

    public void setVersion(int version) {
        mVersion = version;
    }

    /** @return Title currently being displayed in the saved state's current entry. */
    public String getDisplayTitleFromState() {
        return WebContentsStateBridge.getDisplayTitleFromState(this);
    }

    /** @return URL currently being displayed in the saved state's current entry. */
    public String getVirtualUrlFromState() {
        return WebContentsStateBridge.getVirtualUrlFromState(this);
    }
}
