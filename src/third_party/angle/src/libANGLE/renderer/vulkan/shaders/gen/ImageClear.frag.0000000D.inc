// GENERATED FILE - DO NOT EDIT.
// Generated by gen_vk_internal_shaders.py.
//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// shaders/gen/ImageClear.frag.0000000D.inc:
//   Pre-generated shader for the ANGLE Vulkan back-end.

#pragma once
constexpr uint8_t kImageClear_frag_0000000D[] = {
    0x1f,0x8b,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0xff,0x2d,0x50,0xbb,0x0a,0xc2,0x40,
    0x10,0x5c,0x2f,0x9a,0xa7,0xc6,0x04,0x11,0xbb,0x10,0x49,0x29,0x04,0x0b,0x15,0x41,
    0x14,0xac,0x62,0x91,0xce,0x2f,0x10,0x6c,0x2c,0xb4,0xd1,0x8f,0xb7,0x11,0x9c,0x49,
    0x26,0x30,0xbb,0xb7,0xb3,0xb3,0xb3,0x77,0xf1,0x5c,0x15,0x98,0x0d,0x2c,0xb6,0xd0,
    0x72,0xeb,0xbf,0xdc,0x1c,0x18,0xb3,0xc4,0xfc,0x2e,0x37,0xed,0xb5,0xad,0xdf,0x9f,
    0x7b,0xbd,0xd9,0xae,0xd9,0x4f,0xcd,0xeb,0x74,0xec,0x4d,0xa1,0x19,0x22,0x13,0xcf,
    0xdb,0xe3,0x45,0x3e,0x02,0x32,0x68,0xc8,0x05,0x9c,0xc7,0x89,0x5c,0x01,0x8c,0x80,
    0x0b,0x62,0xac,0x5d,0x95,0x72,0x03,0x3d,0x39,0x07,0xcc,0x10,0x99,0x97,0xe0,0x3c,
    0x71,0x73,0x78,0xf8,0xc8,0xa5,0xf6,0x2e,0x50,0xd3,0xdb,0xd7,0xee,0x12,0x31,0x44,
    0xf6,0xb4,0xf3,0xa0,0x3a,0x12,0x57,0xc8,0x3f,0x90,0x36,0x51,0x2f,0x96,0x96,0xf5,
    0x58,0xdc,0x4a,0xbb,0x26,0xba,0x1b,0xf5,0xa9,0x7a,0x9c,0xdf,0xe1,0xfe,0x4e,0x7b,
    0x4d,0xfe,0x5f,0x30,0x7c,0xdb,0x19,0x31,0xed,0xfe,0x4b,0xef,0x47,0x8f,0xa3,0xee,
    0x9a,0x89,0x3f,0x61,0x22,0x52,0xfd,0xc3,0x6b,0xf6,0xc0,0x1f,0x4e,0x42,0xfb,0x94,
    0x88,0x01,0x00,0x00
};

// Generated from:
//
// #version 450 core
//
// layout(push_constant)uniform PushConstants {
//        ivec4 clearColor;
// } params;
//
// layout(location = 5)out ivec4 colorOut;
//
// void main()
// {
//     colorOut = params . clearColor;
// }