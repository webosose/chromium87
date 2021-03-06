// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.messages;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;

/**
 * View representing the message banner.
 */
public class MessageBannerView extends LinearLayout {
    private ImageView mIconView;
    private TextView mTitle;
    private TextView mDescription;
    private TextView mPrimaryButton;
    private ImageView mSecondaryButton;
    private View mDivider;

    public MessageBannerView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mTitle = findViewById(R.id.message_title);
        mDescription = findViewById(R.id.message_description);
        mPrimaryButton = findViewById(R.id.message_primary_button);
        mIconView = findViewById(R.id.message_icon);
        mSecondaryButton = findViewById(R.id.message_secondary_button);
        mDivider = findViewById(R.id.message_divider);
    }

    void setTitle(String title) {
        mTitle.setText(title);
    }

    void setDescription(String description) {
        mDescription.setVisibility(VISIBLE);
        mDescription.setText(description);
    }

    void setIcon(Drawable icon) {
        mIconView.setImageDrawable(icon);
    }

    void setPrimaryButtonText(String text) {
        mPrimaryButton.setText(text);
    }

    void setPrimaryButtonClickListener(OnClickListener listener) {
        mPrimaryButton.setOnClickListener(listener);
    }

    void setSecondaryIcon(Drawable icon) {
        mSecondaryButton.setImageDrawable(icon);
        mSecondaryButton.setVisibility(VISIBLE);
        mDivider.setVisibility(VISIBLE);
    }

    void setSecondaryIconContentDescription(String description) {
        mSecondaryButton.setContentDescription(description);
    }
}
