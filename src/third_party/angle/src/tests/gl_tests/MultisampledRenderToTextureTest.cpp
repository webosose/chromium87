//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// MultisampledRenderToTextureTest: Tests of EXT_multisampled_render_to_texture extension

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
class MultisampledRenderToTextureTest : public ANGLETest
{
  protected:
    MultisampledRenderToTextureTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void testSetUp() override {}

    void testTearDown() override {}

    void assertErrorIfNotMSRTT2(GLenum error)
    {
        if (EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"))
        {
            ASSERT_GL_NO_ERROR();
        }
        else
        {
            ASSERT_GL_ERROR(error);
        }
    }

    void setupCopyTexProgram()
    {
        mCopyTextureProgram.makeRaster(essl1_shaders::vs::Texture2D(),
                                       essl1_shaders::fs::Texture2D());
        ASSERT_GL_TRUE(mCopyTextureProgram.valid());

        mCopyTextureUniformLocation =
            glGetUniformLocation(mCopyTextureProgram, essl1_shaders::Texture2DUniform());

        ASSERT_GL_NO_ERROR();
    }

    void setupUniformColorProgramMultiRenderTarget(const bool bufferEnabled[8], GLuint *programOut)
    {
        std::stringstream fs;

        fs << "#extension GL_EXT_draw_buffers : enable\n"
              "precision highp float;\n"
              "uniform mediump vec4 "
           << essl1_shaders::ColorUniform()
           << ";\n"
              "void main()\n"
              "{\n";

        for (unsigned int index = 0; index < 8; index++)
        {
            if (bufferEnabled[index])
            {
                fs << "    gl_FragData[" << index << "] = " << essl1_shaders::ColorUniform()
                   << ";\n";
            }
        }

        fs << "}\n";

        *programOut = CompileProgram(essl1_shaders::vs::Simple(), fs.str().c_str());
        ASSERT_NE(*programOut, 0u);
    }

    void verifyResults(GLuint texture,
                       const GLColor expected,
                       GLint fboSize,
                       GLint xs,
                       GLint ys,
                       GLint xe,
                       GLint ye)
    {
        glViewport(0, 0, fboSize, fboSize);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Draw a quad with the target texture
        glUseProgram(mCopyTextureProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(mCopyTextureUniformLocation, 0);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_BLEND);
        drawQuad(mCopyTextureProgram, essl1_shaders::PositionAttrib(), 0.5f);

        // Expect that the rendered quad has the same color as the source texture
        EXPECT_PIXEL_COLOR_NEAR(xs, ys, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xs, ye - 1, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xe - 1, ys, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xe - 1, ye - 1, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR((xs + xe) / 2, (ys + ye) / 2, expected, 1.0);
    }

    void clearAndDrawQuad(GLuint program, GLsizei viewportWidth, GLsizei viewportHeight)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, viewportWidth, viewportHeight);
        ASSERT_GL_NO_ERROR();

        drawQuad(program, essl1_shaders::PositionAttrib(), 0.0f);
        ASSERT_GL_NO_ERROR();
    }

    struct GLType
    {
        GLenum internalFormat;
        GLenum format;
        GLenum type;
    };

    void createAndAttachColorAttachment(bool useRenderbuffer,
                                        GLsizei size,
                                        GLenum renderbufferTarget,
                                        const GLType *glType,
                                        GLTexture *textureOut,
                                        GLRenderbuffer *renderbufferOut);
    void createAndAttachDepthStencilAttachment(bool useRenderbuffer,
                                               GLsizei size,
                                               GLTexture *textureOut,
                                               GLRenderbuffer *renderbufferOut);
    void colorAttachmentMultisampleDrawTestCommon(bool useRenderbuffer);
    void copyTexImageTestCommon(bool useRenderbuffer);
    void copyTexSubImageTestCommon(bool useRenderbuffer);
    void drawCopyThenBlendCommon(bool useRenderbuffer);
    void clearDrawCopyThenBlendSameProgramCommon(bool useRenderbuffer);
    void clearThenBlendCommon(bool useRenderbuffer);

    GLProgram mCopyTextureProgram;
    GLint mCopyTextureUniformLocation = -1;
};

class MultisampledRenderToTextureES3Test : public MultisampledRenderToTextureTest
{
  protected:
    void readPixelsTestCommon(bool useRenderbuffer);
    void blitFramebufferTestCommon(bool useRenderbuffer);
    void depthStencilClearThenDrawCommon(bool useRenderbuffer);
    void colorAttachment1Common(bool useRenderbuffer);
    void colorAttachments0And3Common(bool useRenderbuffer);
    void blitFramebufferMixedColorAndDepthCommon(bool useRenderbuffer);
    void renderbufferUnresolveColorAndDepthStencilThenTwoColors(bool withDepth, bool withStencil);
};

class MultisampledRenderToTextureES31Test : public MultisampledRenderToTextureTest
{
  protected:
    void blitFramebufferAttachment1Common(bool useRenderbuffer);
    void drawCopyThenBlendAllAttachmentsMixed(bool useRenderbuffer);
};

// Checking against invalid parameters for RenderbufferStorageMultisampleEXT.
TEST_P(MultisampledRenderToTextureTest, RenderbufferParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

    // Positive test case.  Formats required by the spec (GLES2.0 Table 4.5)
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_NO_ERROR();
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_STENCIL_INDEX8, 64, 64);
    ASSERT_GL_NO_ERROR();
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGBA4, 64, 64);
    ASSERT_GL_NO_ERROR();
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGB5_A1, 64, 64);
    ASSERT_GL_NO_ERROR();
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGB565, 64, 64);
    ASSERT_GL_NO_ERROR();

    // Positive test case.  A few of the ES3 formats
    if (getClientMajorVersion() >= 3)
    {
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, 64, 64);
        ASSERT_GL_NO_ERROR();
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGBA8, 64, 64);
        ASSERT_GL_NO_ERROR();

        if (getClientMinorVersion() >= 1)
        {
            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGBA32I, 64, 64);
            ASSERT_GL_NO_ERROR();
            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_RGBA32UI, 64, 64);
            ASSERT_GL_NO_ERROR();
        }
    }

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    // Samples too large
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples + 1, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Renderbuffer size too large
    GLint maxSize;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 2, GL_RGBA4, maxSize + 1, maxSize);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 2, GL_DEPTH_COMPONENT16, maxSize,
                                        maxSize + 1);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Retrieving samples
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, 64, 64);
    GLint param = 0;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES_EXT, &param);
    // GE because samples may vary base on implementation. Spec says "the resulting value for
    // RENDERBUFFER_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more
    // than the next larger sample count supported by the implementation"
    EXPECT_GE(param, 4);
}

// Checking against invalid parameters for FramebufferTexture2DMultisampleEXT.
TEST_P(MultisampledRenderToTextureTest, Texture2DParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    bool isES3 = getClientMajorVersion() >= 3;

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLTexture depthTexture;
    if (isES3)
    {
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 64, 64, 0, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8_OES, nullptr);
    }

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Positive test case
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Attachment not COLOR_ATTACHMENT0.  Allowed only in EXT_multisampled_render_to_texture2
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         texture, 0, 4);
    assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

    // Depth/stencil attachment.  Allowed only in EXT_multisampled_render_to_texture2
    if (isES3)
    {
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                             GL_TEXTURE_2D, depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);
    }

    // Target not framebuffer
    glFramebufferTexture2DMultisampleEXT(GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    // Samples too large
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, samples + 1);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Retrieving samples
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    GLint param = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT, &param);
    // GE because samples may vary base on implementation. Spec says "the resulting value for
    // TEXTURE_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more than the
    // next larger sample count supported by the implementation"
    EXPECT_GE(param, 4);
}

// Checking against invalid parameters for FramebufferTexture2DMultisampleEXT (cubemap).
TEST_P(MultisampledRenderToTextureTest, TextureCubeMapParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLTexture texture;
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    for (GLenum face = 0; face < 6; face++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA, 64, 64, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        ASSERT_GL_NO_ERROR();
    }

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (GLenum face = 0; face < 6; face++)
    {
        // Positive test case
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        ASSERT_GL_NO_ERROR();
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

        // Attachment not COLOR_ATTACHMENT0.  Allowed only in EXT_multisampled_render_to_texture2
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        // Target not framebuffer
        glFramebufferTexture2DMultisampleEXT(GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        ASSERT_GL_ERROR(GL_INVALID_ENUM);

        // Samples too large
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0,
                                             samples + 1);
        ASSERT_GL_ERROR(GL_INVALID_VALUE);

        // Retrieving samples
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        GLint param = 0;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT,
                                              &param);
        // GE because samples may vary base on implementation. Spec says "the resulting value for
        // TEXTURE_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more than
        // the next larger sample count supported by the implementation"
        EXPECT_GE(param, 4);
    }
}

// Checking for framebuffer completeness using extension methods.
TEST_P(MultisampledRenderToTextureTest, FramebufferCompleteness)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // Checking that Renderbuffer and texture2d having different number of samples results
    // in a FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    // Texture attachment for color attachment 0.  Framebuffer should be complete.
    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    // Depth/stencil renderbuffer, potentially with a different sample count.
    GLRenderbuffer dsRenderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, dsRenderbuffer);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, maxSamples, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_NO_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dsRenderbuffer);

    if (maxSamples > 4)
    {
        EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
                         glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
    else
    {
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    }

    // Color renderbuffer for color attachment 0.
    GLRenderbuffer colorRenderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, maxSamples, GL_RGBA4, 64, 64);
    ASSERT_GL_NO_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              colorRenderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // D3D backend doesn't implement multisampled render to texture renderbuffers correctly.
    // http://anglebug.com/3107
    ANGLE_SKIP_TEST_IF(IsD3D());

    if (getClientMajorVersion() >= 3)
    {
        // Texture attachment for color attachment 1.
        GLTexture texture2;
        glBindTexture(GL_TEXTURE_2D, texture2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        ASSERT_GL_NO_ERROR();

        // Attach with a potentially different number of samples.
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                             texture, 0, 4);

        if (maxSamples > 4)
        {
            EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
                             glCheckFramebufferStatus(GL_FRAMEBUFFER));
        }
        else
        {
            EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
        }

        // Attach with same number of samples.
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                             texture, 0, maxSamples);
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    }
}

void MultisampledRenderToTextureTest::createAndAttachColorAttachment(
    bool useRenderbuffer,
    GLsizei size,
    GLenum renderbufferTarget,
    const GLType *glType,
    GLTexture *textureOut,
    GLRenderbuffer *renderbufferOut)
{
    GLenum internalFormat = glType ? glType->internalFormat : GL_RGBA;
    GLenum format         = glType ? glType->format : GL_RGBA;
    GLenum type           = glType ? glType->type : GL_UNSIGNED_BYTE;

    if (useRenderbuffer)
    {
        if (internalFormat == GL_RGBA)
        {
            internalFormat = GL_RGBA8;
        }
        glBindRenderbuffer(GL_RENDERBUFFER, *renderbufferOut);
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, internalFormat, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, renderbufferTarget, GL_RENDERBUFFER,
                                  *renderbufferOut);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, *textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size, size, 0, format, type, nullptr);
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, renderbufferTarget, GL_TEXTURE_2D,
                                             *textureOut, 0, 4);
    }
    ASSERT_GL_NO_ERROR();
}

void MultisampledRenderToTextureTest::createAndAttachDepthStencilAttachment(
    bool useRenderbuffer,
    GLsizei size,
    GLTexture *textureOut,
    GLRenderbuffer *renderbufferOut)
{
    if (useRenderbuffer)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, *renderbufferOut);
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  *renderbufferOut);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, *textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size, size, 0, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8_OES, nullptr);
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                             GL_TEXTURE_2D, *textureOut, 0, 4);
    }
    ASSERT_GL_NO_ERROR();
}

void MultisampledRenderToTextureTest::colorAttachmentMultisampleDrawTestCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Set up color attachment and bind to FBO
    constexpr GLsizei kSize = 6;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr, &texture,
                                   &renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear to black
    glViewport(0, 0, kSize, kSize);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up Green square program
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(program);
    GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    setupQuadVertexBuffer(0.5f, 0.5f);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Draw green square
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    // Set up Red square program
    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program2);
    GLint positionLocation2 = glGetAttribLocation(program2, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation2);

    setupQuadVertexBuffer(0.5f, 0.75f);
    glVertexAttribPointer(positionLocation2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Draw red square
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw test with color attachment only.
TEST_P(MultisampledRenderToTextureTest, 2DColorAttachmentMultisampleDrawTest)
{
    colorAttachmentMultisampleDrawTestCommon(false);
}

// Draw test with renderbuffer color attachment only
TEST_P(MultisampledRenderToTextureTest, RenderbufferColorAttachmentMultisampleDrawTest)
{
    colorAttachmentMultisampleDrawTestCommon(true);
}

// Draw test using both color and depth attachments.
TEST_P(MultisampledRenderToTextureTest, 2DColorDepthMultisampleDrawTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 6;
    // create complete framebuffer with depth buffer
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);

    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, kSize, kSize);
    ASSERT_GL_NO_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear framebuffer
    glViewport(0, 0, kSize, kSize);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearDepthf(0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw first green square
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glUseProgram(program);
    GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    setupQuadVertexBuffer(0.8f, 0.5f);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Tests that TRIANGLES works.
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    // Draw red square behind green square
    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program2);
    GLint positionLocation2 = glGetAttribLocation(program2, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation2);

    setupQuadVertexBuffer(0.7f, 1.0f);
    glVertexAttribPointer(positionLocation2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();
    glDisable(GL_DEPTH_TEST);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MultisampledRenderToTextureES3Test::readPixelsTestCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    constexpr GLsizei kSize = 6;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr, &texture,
                                   &renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear to red
    glViewport(0, 0, kSize, kSize);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Bind Pack Pixel Buffer and read to it
    GLBuffer PBO;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_PACK_BUFFER, 4 * kSize * kSize, nullptr, GL_STATIC_DRAW);
    glReadPixels(0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASSERT_GL_NO_ERROR();

    // Retrieving pixel color
    void *mappedPtr    = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 32, GL_MAP_READ_BIT);
    GLColor *dataColor = static_cast<GLColor *>(mappedPtr);
    EXPECT_GL_NO_ERROR();

    EXPECT_EQ(GLColor::red, dataColor[0]);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    EXPECT_GL_NO_ERROR();
}

// Read pixels with pack buffer. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, ReadPixelsTest)
{
    readPixelsTestCommon(false);
}

// Read pixels with pack buffer from renderbuffer. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferReadPixelsTest)
{
    // D3D backend doesn't implement multisampled render to texture renderbuffers correctly.
    // http://anglebug.com/3107
    ANGLE_SKIP_TEST_IF(IsD3D());

    readPixelsTestCommon(true);
}

void MultisampledRenderToTextureTest::copyTexImageTestCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 16;

    setupCopyTexProgram();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    GLTexture texture;
    GLRenderbuffer renderbuffer;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr, &texture,
                                   &renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set color for framebuffer
    glClearColor(0.25f, 1.0f, 0.75f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    GLTexture copyToTex;
    glBindTexture(GL_TEXTURE_2D, copyToTex);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, kSize, kSize, 0);
    ASSERT_GL_NO_ERROR();

    verifyResults(copyToTex, {64, 255, 191, 255}, kSize, 0, 0, kSize, kSize);
}

// CopyTexImage from a multisampled texture functionality test.
TEST_P(MultisampledRenderToTextureTest, CopyTexImageTest)
{
    copyTexImageTestCommon(false);
}

// CopyTexImage from a multisampled texture functionality test using renderbuffer.
TEST_P(MultisampledRenderToTextureTest, RenderbufferCopyTexImageTest)
{
    copyTexImageTestCommon(true);
}

void MultisampledRenderToTextureTest::copyTexSubImageTestCommon(bool useRenderbuffer)
{
    // Fails on Pixel 2. http://anglebug.com/4906
    ANGLE_SKIP_TEST_IF(IsAndroid());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 16;

    setupCopyTexProgram();

    GLFramebuffer copyFBO0;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO0);

    // Create color attachment for copyFBO0
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr, &texture,
                                   &renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    GLFramebuffer copyFBO1;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO1);

    // Create color attachment for copyFBO1
    GLTexture texture1;
    GLRenderbuffer renderbuffer1;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr, &texture1,
                                   &renderbuffer1);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set color for copyFBO0
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO0);
    glClearColor(0.25f, 1.0f, 0.75f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Set color for copyFBO1
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO1);
    glClearColor(1.0f, 0.75f, 0.5f, 0.25f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    GLTexture copyToTex;
    glBindTexture(GL_TEXTURE_2D, copyToTex);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // copyFBO0 -> copyToTex
    // copyToTex should hold what was originally in copyFBO0 : (.25, 1, .75, .5)
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO0);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, kSize, kSize, 0);
    ASSERT_GL_NO_ERROR();

    const GLColor expected0(64, 255, 191, 255);
    verifyResults(copyToTex, expected0, kSize, 0, 0, kSize, kSize);

    // copyFBO[1] - copySubImage -> copyToTex
    // copyToTex should have subportion what was in copyFBO[1] : (1, .75, .5, .25)
    // The rest should still be untouched: (.25, 1, .75, .5)
    GLint half = kSize / 2;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO1);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, half, half, half, half, half, half);
    ASSERT_GL_NO_ERROR();

    const GLColor expected1(255, 191, 127, 255);
    verifyResults(copyToTex, expected1, kSize, half, half, kSize, kSize);

    // Verify rest is untouched
    verifyResults(copyToTex, expected0, kSize, 0, 0, half, half);
    verifyResults(copyToTex, expected0, kSize, 0, half, half, kSize);
    verifyResults(copyToTex, expected0, kSize, half, 0, kSize, half);
}

// CopyTexSubImage from a multisampled texture functionality test.
TEST_P(MultisampledRenderToTextureTest, CopyTexSubImageTest)
{
    copyTexSubImageTestCommon(false);
}

// CopyTexSubImage from a multisampled texture functionality test with renderbuffers
TEST_P(MultisampledRenderToTextureTest, RenderbufferCopyTexSubImageTest)
{
    copyTexSubImageTestCommon(true);
}

void MultisampledRenderToTextureES3Test::blitFramebufferTestCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    constexpr GLsizei kSize = 16;

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to use as source.
    GLRenderbuffer depthMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthMS);
    ASSERT_GL_NO_ERROR();

    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear depth to 0.5 and color to green.
    glClearDepthf(0.5f);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glFlush();
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawRed, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    drawQuad(drawRed, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();
}

// BlitFramebuffer functionality test. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, BlitFramebufferTest)
{
    blitFramebufferTestCommon(false);
}

// BlitFramebuffer functionality test with renderbuffer. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferBlitFramebufferTest)
{
    blitFramebufferTestCommon(true);
}

// GenerateMipmap functionality test
TEST_P(MultisampledRenderToTextureTest, GenerateMipmapTest)
{
    // Fails on Pixel 2. http://anglebug.com/4906
    ANGLE_SKIP_TEST_IF(IsAndroid());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();
    glUseProgram(mCopyTextureProgram);

    // Initialize texture with blue
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kSize, kSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, kSize, kSize);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ASSERT_GL_NO_ERROR();

    // Generate mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    ASSERT_GL_NO_ERROR();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

    // Now draw the texture to various different sized areas.
    clearAndDrawQuad(mCopyTextureProgram, kSize, kSize);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    // Use mip level 1
    clearAndDrawQuad(mCopyTextureProgram, kSize / 2, kSize / 2);
    EXPECT_PIXEL_COLOR_EQ(kSize / 4, kSize / 4, GLColor::blue);

    // Use mip level 2
    clearAndDrawQuad(mCopyTextureProgram, kSize / 4, kSize / 4);
    EXPECT_PIXEL_COLOR_EQ(kSize / 8, kSize / 8, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

void MultisampledRenderToTextureTest::drawCopyThenBlendCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Draw again into the framebuffer, this time blending.  This tests that the framebuffer's data,
    // residing in the single-sampled texture, is available to the multisampled intermediate image
    // for blending.

    // Blend half-transparent green into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    ASSERT_GL_NO_ERROR();
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
TEST_P(MultisampledRenderToTextureTest, DrawCopyThenBlend)
{
    drawCopyThenBlendCommon(false);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureTest, RenderbufferDrawCopyThenBlend)
{
    drawCopyThenBlendCommon(true);
}

void MultisampledRenderToTextureTest::clearDrawCopyThenBlendSameProgramCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Clear the framebuffer.
    glClearColor(0.1f, 0.9f, 0.2f, 0.8f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Then draw into it.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Draw again into the framebuffer, this time blending.  This tests that the framebuffer's data,
    // residing in the single-sampled texture, is available to the multisampled intermediate image
    // for blending.

    // Blend half-transparent green into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // Once again, clear and draw so the program is used again in the way it was first used.
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);

    EXPECT_PIXEL_COLOR_NEAR(0, 0, GLColor::blue, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, GLColor::blue, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, GLColor::blue, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, GLColor::blue, 1);

    // For completeness, verify that the texture used as copy target is red.
    ASSERT_GL_NO_ERROR();
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();
}

// Clear&Draw, copy, then blend with same program.  The copy will make sure an implicit resolve
// happens.  The second draw should retain the data written by the first draw command ("unresolve"
// operation).  The same program is used for the first and second draw calls, and the fact that the
// attachment is cleared or unresolved should not cause issues.  In the Vulkan backend, the program
// will be used in different subpass indices, so two graphics pipelines should be created for it.
TEST_P(MultisampledRenderToTextureTest, ClearDrawCopyThenBlendSameProgram)
{
    clearDrawCopyThenBlendSameProgramCommon(false);
}

// Same as ClearDrawCopyThenBlendSameProgram but with renderbuffers
TEST_P(MultisampledRenderToTextureTest, RenderbufferClearDrawCopyThenBlendSameProgram)
{
    clearDrawCopyThenBlendSameProgramCommon(true);
}

void MultisampledRenderToTextureTest::clearThenBlendCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear the framebuffer.
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Blend half-transparent green into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);
}

// Clear then blend.  The clear should be applied correctly.
TEST_P(MultisampledRenderToTextureTest, ClearThenBlend)
{
    clearThenBlendCommon(false);
}

// Clear then blend.  The clear should be applied correctly.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureTest, RenderbufferClearThenBlend)
{
    clearThenBlendCommon(true);
}

void MultisampledRenderToTextureES3Test::depthStencilClearThenDrawCommon(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    if (!useRenderbuffer)
    {
        ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));
    }

    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create framebuffer to draw into, with both color and depth attachments.
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);

    GLTexture dsTextureMS;
    GLRenderbuffer dsRenderbufferMS;
    createAndAttachDepthStencilAttachment(useRenderbuffer, kSize, &dsTextureMS, &dsRenderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear depth/stencil
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not cleared to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawRed, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());

    // Draw red
    drawQuad(drawRed, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Verify.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);
}

// Clear depth stencil, then draw.  The clear should be applied correctly.
TEST_P(MultisampledRenderToTextureES3Test, DepthStencilClearThenDraw)
{
    depthStencilClearThenDrawCommon(false);
}

// Clear depth stencil, then draw.  The clear should be applied correctly.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferDepthStencilClearThenDraw)
{
    depthStencilClearThenDrawCommon(true);
}

// Clear&Draw, copy, then blend similarly to RenderbufferClearDrawCopyThenBlendSameProgram.  This
// tests uses a depth/stencil buffer and makes sure the second draw (in the second render pass)
// succeeds (i.e. depth/stencil data is not lost).  Note that this test doesn't apply to
// depth/stencil textures as they are explicitly autoinvalidated between render passes.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferDepthStencilClearDrawCopyThenBlend)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    // http://anglebug.com/5096
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create framebuffer to draw into, with both color and depth/stencil attachments.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear depth/stencil
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not cleared to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.25f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Draw again into the framebuffer, this time blending.  This tests that both the color and
    // depth/stencil data are preserved after the resolve incurred by the copy above.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);
}

// Draw, copy, then clear&blend.  This tests uses a depth/stencil buffer and makes sure the second
// draw (in the second render pass) succeeds (i.e.  depth/stencil data is not lost).  The difference
// with RenderbufferDepthStencilClearDrawCopyThenBlend is that color is cleared in the second render
// pass, so only depth/stencil data is unresolved.  This test doesn't apply to depth/stencil
// textures as they are explicitly autoinvalidated between render passes.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferDepthStencilDrawCopyClearThenBlend)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    // http://anglebug.com/5096
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create framebuffer to draw into, with both color and depth/stencil attachments.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear depth/stencil through draw
    glViewport(0, 0, kSize, kSize);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0x55, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 1.0f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Clear color to blue
    glClearColor(0.0, 0.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not cleared to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Draw again into the framebuffer, this time blending.  This tests that depth/stencil data are
    // preserved after the resolve incurred by the copy above and color is cleared correctly.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Copy into the texture again.
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now cyan
    const GLColor kExpected(0, 127, 127, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is also cyan.
    const GLColor expectedCopyResult(0, 127, 127, 191);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);
}

// Clear, then blit depth/stencil with renderbuffers.  This test makes sure depth/stencil blit uses
// the correct image.  Note that this test doesn't apply to depth/stencil textures as they are
// explicitly autoinvalidated between render passes.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferClearThenBlitDepthStencil)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // D3D backend doesn't implement multisampled render to texture renderbuffers correctly.
    // http://anglebug.com/3107
    ANGLE_SKIP_TEST_IF(IsD3D());

    // The following trybot configurations don't support VK_KHR_depth_stencil_resolve.  ANGLE is not
    // conformant without this extension, but it's allowed as users commonly invalidate
    // depth/stencil.
    //
    //  - SwifthShader
    //  - Android
    ANGLE_SKIP_TEST_IF(IsVulkan() && (isSwiftshader() || IsAndroid() || IsAMD()));

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create framebuffer to draw into, with both color and depth/stencil attachments.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLRenderbuffer depthStencilMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencilMS);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear depth/stencil
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Create framebuffer as blit target.
    GLFramebuffer fbo;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_DRAW_FRAMEBUFFER);

    // Blit depth/stencil
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    // Draw into the framebuffer that was the destination of blit, verifying that depth and stencil
    // values are correct.

    // If depth is not 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now red
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);

    // Clear depth/stencil to a different value, and blit again but this time flipped.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboMS);
    glClearDepthf(0);
    glClearStencil(0x3E);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Blit
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, kSize, kSize, kSize, kSize, 0, 0,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    // Draw green
    glDepthFunc(GL_GREATER);
    glStencilFunc(GL_EQUAL, 0x3E, 0xFF);
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now green
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::green);
}

// Draw, then blit depth/stencil with renderbuffers.  This test makes sure depth/stencil resolve is
// correctly implemented.  Note that this test doesn't apply to depth/stencil textures as they are
// explicitly autoinvalidated between render passes.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferDrawThenBlitDepthStencil)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // Skip on configurations that don't support VK_KHR_depth_stencil_resolve.  See comment
    // in RenderbufferClearThenBlitDepthStencil.
    ANGLE_SKIP_TEST_IF(IsVulkan() && (isSwiftshader() || IsAndroid() || IsAMD()));

    // http://anglebug.com/5096
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create framebuffer to draw into, with both color and depth/stencil attachments.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLRenderbuffer depthStencilMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencilMS);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Output depth/stencil through draw
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0x55, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);

    // Draw blue
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 1.0f);
    ASSERT_GL_NO_ERROR();

    // Create framebuffer as blit target.
    GLFramebuffer fbo;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_DRAW_FRAMEBUFFER);

    // Blit depth/stencil
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    // Draw into the framebuffer that was the destination of blit, verifying that depth and stencil
    // values are correct.

    // If depth is not 1, rendering would fail.
    glDepthFunc(GL_LESS);

    // If stencil is not 0x55, rendering would fail.
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now red
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
}

// Draw, then blit depth/stencil with renderbuffers, without a color attachment. Note that this test
// doesn't apply to depth/stencil textures as they are explicitly autoinvalidated between render
// passes.
//
// This test first uses a draw call to fill in the depth/stencil buffer, then blits it to force a
// resolve.  Then it uses a no-op draw call to start a "fullscreen" render pass followed by a
// scissored draw to modify parts of the depth buffer:
//
// +--------------------+
// |     D=1, S=0x55    |
// |                    |
// |     +--------+     |
// |     |  D=0   |     |
// |     | S=0xAA |     |
// |     +--------+     |
// |                    |
// |                    |
// +--------------------+
//
// Blit is used again to copy the depth/stencil attachment data, and the result is verified.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferDrawThenBlitDepthStencilOnly)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // Skip on configurations that don't support VK_KHR_depth_stencil_resolve.  See comment
    // in RenderbufferClearThenBlitDepthStencil.
    ANGLE_SKIP_TEST_IF(IsVulkan() && (isSwiftshader() || IsAndroid() || IsAMD()));

    // http://anglebug.com/5096
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    // http://anglebug.com/5110
    ANGLE_SKIP_TEST_IF(IsD3D());

    constexpr GLsizei kSize = 64;

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create framebuffer to draw into, with depth/stencil attachment only.
    GLRenderbuffer depthStencilMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencilMS);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);

    // Output depth/stencil through draw
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0x55, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);

    // Draw.  Depth/stencil is now:
    //
    // +--------------------+
    // |     D=1, S=0x55    |
    // |                    |
    // |                    |
    // |                    |
    // |                    |
    // |                    |
    // |                    |
    // |                    |
    // +--------------------+
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 1.0f);
    ASSERT_GL_NO_ERROR();

    // Create framebuffer as blit target.
    GLFramebuffer fbo;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kSize, kSize);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_DRAW_FRAMEBUFFER);

    // Blit depth/stencil
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    // Disable depth/stencil and draw again.  Depth/stencil is not modified.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboMS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Enable depth/stencil and do a scissored draw.  Depth/stencil is now:
    //
    // +--------------------+
    // |     D=1, S=0x55    |
    // |                    |
    // |     +--------+     |
    // |     |  D=0   |     |
    // |     | S=0xAA |     |
    // |     +--------+     |
    // |                    |
    // |                    |
    // +--------------------+
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0xAA, 0xFF);
    glEnable(GL_SCISSOR_TEST);
    glScissor(kSize / 4, kSize / 4, kSize / 2, kSize / 2);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), -1.0f);
    glDisable(GL_SCISSOR_TEST);
    ASSERT_GL_NO_ERROR();

    // Blit depth/stencil again.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Draw into the framebuffer that was the destination of blit, verifying that depth and stencil
    // values are correct.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_DRAW_FRAMEBUFFER);

    // First, verify the outside border, where D=1 and S=0x55
    glDepthFunc(GL_LESS);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
    ASSERT_GL_NO_ERROR();

    // Draw green
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.95f);
    ASSERT_GL_NO_ERROR();

    // Then, verify the center, where D=0 and S=0xAA
    glDepthFunc(GL_GREATER);
    glStencilFunc(GL_EQUAL, 0xAA, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Draw blue
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), -0.95f);
    ASSERT_GL_NO_ERROR();

    // Verify that the border is now green
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::green);

    // Verify that the center is now blue
    EXPECT_PIXEL_COLOR_EQ(kSize / 4, kSize / 4, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 4 - 1, kSize / 4, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 4, 3 * kSize / 4 - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 4 - 1, 3 * kSize / 4 - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);
}

// Test the depth read/write mode change within the renderpass while there is color unresolve
// attachment
TEST_P(MultisampledRenderToTextureTest, DepthReadWriteToggleWithStartedRenderPass)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create framebuffer to draw into, with both color and depth attachments.
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(true, kSize, GL_COLOR_ATTACHMENT0, nullptr, &textureMS,
                                   &renderbufferMS);

    GLTexture dsTextureMS;
    GLRenderbuffer dsRenderbufferMS;
    glBindRenderbuffer(GL_RENDERBUFFER, dsRenderbufferMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              dsRenderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // First renderpass: draw with depth value 0.5f
    glViewport(0, 0, kSize, kSize);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    ANGLE_GL_PROGRAM(drawBlue, essl1_shaders::vs::Simple(), essl1_shaders::fs::Blue());
    drawQuad(drawBlue, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    // The color check should end the renderpass
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::blue);

    // Create another FBO and render, jus so try to clear rendering cache. At least on pixel4,
    // the test now properly fail if I force the loadOP to DontCare in the next renderpass.
    constexpr bool clearRenderingCacheWithFBO = true;
    if (clearRenderingCacheWithFBO)
    {
        GLFramebuffer fboMS2;
        glBindFramebuffer(GL_FRAMEBUFFER, fboMS2);
        GLTexture textureMS2;
        GLRenderbuffer renderbufferMS2;
        createAndAttachColorAttachment(true, 2048, GL_COLOR_ATTACHMENT0, nullptr, &textureMS2,
                                       &renderbufferMS2);
        GLTexture dsTextureMS2;
        GLRenderbuffer dsRenderbufferMS2;
        glBindRenderbuffer(GL_RENDERBUFFER, dsRenderbufferMS2);
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, 2048, 2048);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                  dsRenderbufferMS2);
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
        ASSERT_GL_NO_ERROR();
        glViewport(0, 0, 2048, 2048);
        ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
        glUseProgram(drawColor);
        GLint colorUniformLocation =
            glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
        ASSERT_NE(colorUniformLocation, -1);
        glUniform4f(colorUniformLocation, 0.0f, 0.0f, 0.0f, 0.0f);
        drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
        ASSERT_GL_NO_ERROR();
    }

    // Second renderpass: Start with depth read only and then switch to depth write
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glViewport(0, 0, kSize, kSize);
    glDepthFunc(GL_LESS);
    // Draw red with depth read only. pass depth test, Result: color=Red, depth=0.5
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ONE, GL_ONE);
    ANGLE_GL_PROGRAM(drawRed, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(drawRed);
    drawQuad(drawRed, essl1_shaders::PositionAttrib(), 0.1f);
    ASSERT_GL_NO_ERROR();

    // Draw green with depth write. Pass depth test. Result: color=Green, depth=0.3
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE, GL_ONE);
    ANGLE_GL_PROGRAM(drawGreen, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(drawGreen);
    drawQuad(drawGreen, essl1_shaders::PositionAttrib(), 0.3f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Verify the color has all three color in it.
    const GLColor expectedCopyResult(255, 255, 255, 255);
    EXPECT_PIXEL_COLOR_EQ(1, 1, expectedCopyResult);
}

void MultisampledRenderToTextureES3Test::colorAttachment1Common(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    // Qualcomm driver crashes in the presence of VK_ATTACHMENT_UNUSED.
    // http://anglebug.com/3423
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAndroid());

    // Fails on Intel Ubuntu 19.04 Mesa 19.0.2 Vulkan. http://anglebug.com/3616
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT1, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Setup program to render into attachment 1.
    constexpr bool kBuffersEnabled[8] = {false, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachment 1.
TEST_P(MultisampledRenderToTextureES3Test, ColorAttachment1)
{
    colorAttachment1Common(false);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachment 1.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferColorAttachment1)
{
    colorAttachment1Common(true);
}

void MultisampledRenderToTextureES3Test::colorAttachments0And3Common(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!useRenderbuffer &&
                       !EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    // Qualcomm driver crashes in the presence of VK_ATTACHMENT_UNUSED.
    // http://anglebug.com/3423
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAndroid());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture textureMS0;
    GLRenderbuffer renderbufferMS0;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS0, &renderbufferMS0);

    GLTexture textureMS3;
    GLRenderbuffer renderbufferMS3;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT3, nullptr,
                                   &textureMS3, &renderbufferMS3);

    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Setup program to render into attachments 0 and 3.
    constexpr bool kBuffersEnabled[8] = {true, false, false, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE,
                                       GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy from one of them.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the textures are now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachments 0 and 3.
TEST_P(MultisampledRenderToTextureES3Test, ColorAttachments0And3)
{
    colorAttachments0And3Common(false);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachments 0 and 3.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferColorAttachments0And3)
{
    colorAttachments0And3Common(true);
}

// Draw with depth buffer.  Uses EXT_multisampled_render_to_texture2.
// The test works with a 64x1 texture.  The first draw call will render geometry whose depth is
// different between top and bottom.  The second draw call will enable depth test and draw with the
// average of the two depths.  Only half of the samples will take the new color.  Once resolved, the
// expected color would be the average of the two draw colors.
TEST_P(MultisampledRenderToTextureES3Test, DepthStencilAttachment)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kWidth = 64;

    // Create multisampled framebuffer to draw into, with both color and depth attachments.
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kWidth, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLTexture depthMS;
    glBindTexture(GL_TEXTURE_2D, depthMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kWidth, 1, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8_OES, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                         depthMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup draw program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);
    GLint positionLocation = glGetAttribLocation(drawColor, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    // Setup vertices such that depth is varied from top to bottom.
    std::array<Vector3, 6> quadVertices = {
        Vector3(-1.0f, 1.0f, 0.8f), Vector3(-1.0f, -1.0f, 0.2f), Vector3(1.0f, -1.0f, 0.2f),
        Vector3(-1.0f, 1.0f, 0.8f), Vector3(1.0f, -1.0f, 0.2f),  Vector3(1.0f, 1.0f, 0.8f),
    };
    GLBuffer quadVertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 6, quadVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Draw red into the framebuffer.
    glViewport(0, 0, kWidth, 1);
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Draw green such that half the samples of each pixel pass the depth test.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    glDepthFunc(GL_GREATER);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    const GLColor kExpected(127, 127, 0, 255);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kWidth - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kWidth / 2, 0, kExpected, 1);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachments 0 and 1.  Attachment 0 is a normal multisampled texture, while attachment
// 1 is a multisampled-render-to-texture texture.
TEST_P(MultisampledRenderToTextureES31Test, MixedMultisampledAndMultisampledRenderToTexture)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS0;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorMS0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, kSize, kSize, true);

    GLTexture colorMS1;
    glBindTexture(GL_TEXTURE_2D, colorMS1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           colorMS0, 0);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         colorMS1, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup program to render into attachments 0 and 1.
    constexpr bool kBuffersEnabled[8] = {true, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy from one of them.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the textures are now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

void MultisampledRenderToTextureES31Test::blitFramebufferAttachment1Common(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!useRenderbuffer &&
                       !EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kSize = 16;

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS0;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorMS0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, kSize, kSize, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           colorMS0, 0);

    GLTexture textureMS1;
    GLRenderbuffer renderbufferMS1;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT1, nullptr,
                                   &textureMS1, &renderbufferMS1);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Setup program to render into attachments 0 and 1.
    constexpr bool kBuffersEnabled[8] = {true, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();
}

// BlitFramebuffer functionality test with mixed color attachments where multisampled render to
// texture as attachment 1 and is the read buffer.  This test makes sure the fact that attachment 0
// is a true multisampled texture doesn't cause issues.
// Uses EXT_multisampled_render_to_texture2.
TEST_P(MultisampledRenderToTextureES31Test, BlitFramebufferAttachment1)
{
    blitFramebufferAttachment1Common(false);
}

// BlitFramebuffer functionality test with mixed color attachments where multisampled render to
// texture as attachment 1 and is the read buffer.  This test makes sure the fact that attachment 0
// is a true multisampled texture doesn't cause issues.
// Uses renderbuffer.
TEST_P(MultisampledRenderToTextureES31Test, RenderbufferBlitFramebufferAttachment1)
{
    blitFramebufferAttachment1Common(true);
}

void MultisampledRenderToTextureES3Test::blitFramebufferMixedColorAndDepthCommon(
    bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    constexpr GLsizei kSize = 16;

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    // Create multisampled framebuffer to use as source.
    GLRenderbuffer depthMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthMS);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthMS);

    GLTexture textureMS;
    GLRenderbuffer renderbufferMS;
    createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0, nullptr,
                                   &textureMS, &renderbufferMS);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear depth to 0.5 and color to red.
    glClearDepthf(0.5f);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLRenderbuffer depthSS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthSS);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kSize, kSize);

    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();

    // Use a small shader to verify depth.
    ANGLE_GL_PROGRAM(depthTestProgram, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::Blue());
    ANGLE_GL_PROGRAM(depthTestProgramFail, essl1_shaders::vs::Passthrough(),
                     essl1_shaders::fs::Green());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    drawQuad(depthTestProgram, essl1_shaders::PositionAttrib(), -0.01f);
    drawQuad(depthTestProgramFail, essl1_shaders::PositionAttrib(), 0.01f);
    glDisable(GL_DEPTH_TEST);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);
    ASSERT_GL_NO_ERROR();
}

// BlitFramebuffer functionality test with mixed multisampled-render-to-texture color attachment and
// multisampled depth buffer.  This test makes sure that the color attachment is blitted, while
// the depth/stencil attachment is resolved.
TEST_P(MultisampledRenderToTextureES3Test, BlitFramebufferMixedColorAndDepth)
{
    blitFramebufferMixedColorAndDepthCommon(false);
}

// BlitFramebuffer functionality test with mixed multisampled-render-to-texture color attachment and
// multisampled depth buffer.  This test makes sure that the color attachment is blitted, while
// the depth/stencil attachment is resolved.  Uses renderbuffer.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferBlitFramebufferMixedColorAndDepth)
{
    blitFramebufferMixedColorAndDepthCommon(true);
}

// Draw non-multisampled, draw multisampled, repeat.  This tests the same texture being bound
// differently to two FBOs.
TEST_P(MultisampledRenderToTextureTest, DrawNonMultisampledThenMultisampled)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    // http://anglebug.com/4935
    ANGLE_SKIP_TEST_IF(IsD3D11());

    // Texture attachment to the two framebuffers.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create singlesampled framebuffer.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    ASSERT_GL_NO_ERROR();

    // Create multisampled framebuffer.
    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw green into the singlesampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glEnable(GL_SCISSOR_TEST);
    glScissor(kSize / 8, kSize / 8, 3 * kSize / 4, 3 * kSize / 4);
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw blue into the multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glScissor(kSize / 4, kSize / 4, kSize / 2, kSize / 2);
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is red on the border, blue in the middle and green in between.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 13 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 13 * kSize / 16, GLColor::green);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

// Draw multisampled, draw multisampled with another sample count, repeat.  This tests the same
// texture being bound as multisampled-render-to-texture with different sample counts to two FBOs.
TEST_P(MultisampledRenderToTextureTest, DrawMultisampledDifferentSamples)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    ASSERT_GE(maxSamples, 4);

    // Texture attachment to the two framebuffers.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create two multisampled framebuffers.
    GLFramebuffer fboMS1;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS1);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLFramebuffer fboMS2;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS2);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, maxSamples);
    ASSERT_GL_NO_ERROR();

    // Draw red into the first multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw green into the second multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS1);
    glEnable(GL_SCISSOR_TEST);
    glScissor(kSize / 8, kSize / 8, 3 * kSize / 4, 3 * kSize / 4);
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw blue into the first multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS2);
    glScissor(kSize / 4, kSize / 4, kSize / 2, kSize / 2);
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is red on the border, blue in the middle and green in between.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 13 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 13 * kSize / 16, GLColor::green);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

void MultisampledRenderToTextureES31Test::drawCopyThenBlendAllAttachmentsMixed(bool useRenderbuffer)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    GLint maxDrawBuffers = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    // At least 4 draw buffers per GLES3.0 spec.
    ASSERT_GE(maxDrawBuffers, 4);

    // Maximum 8 draw buffers exposed by ANGLE.
    constexpr GLint kImplMaxDrawBuffers = 8;
    maxDrawBuffers                      = std::min(maxDrawBuffers, kImplMaxDrawBuffers);

    constexpr const char *kDecl[kImplMaxDrawBuffers] = {
        "layout(location = 0) out vec4 out0;",  "layout(location = 1) out ivec4 out1;",
        "layout(location = 2) out uvec4 out2;", "layout(location = 3) out vec4 out3;",
        "layout(location = 4) out uvec4 out4;", "layout(location = 5) out ivec4 out5;",
        "layout(location = 6) out ivec4 out6;", "layout(location = 7) out vec4 out7;",
    };

    constexpr GLType kGLType[kImplMaxDrawBuffers] = {
        {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},           {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT},
        {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT}, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
        {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT}, {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT},
        {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT},           {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
    };

    constexpr const char *kAssign1[kImplMaxDrawBuffers] = {
        "out0 = vec4(1.0f, 0.0f, 0.0f, 1.0f);",
        "out1 = ivec4(-19, 13, 123456, -654321);",
        "out2 = uvec4(98765, 43210, 2, 0);",
        "out3 = vec4(0.0f, 1.0f, 0.0f, 1.0f);",
        "out4 = uvec4(10101010, 2345, 0, 991);",
        "out5 = ivec4(615243, -948576, -222, 111);",
        "out6 = ivec4(-8127931, -1392781, 246810, 1214161820);",
        "out7 = vec4(0.0f, 0.0f, 1.0f, 1.0f);",
    };

    constexpr const char *kAssign2[kImplMaxDrawBuffers] = {
        "out0 = vec4(0.0f, 1.0f, 0.0f, 0.5f);",
        "out1 = ivec4(0, 0, 0, 0);",
        "out2 = uvec4(0, 0, 0, 0);",
        "out3 = vec4(0.0f, 0.0f, 1.0f, 0.5f);",
        "out4 = uvec4(0, 0, 0, 0);",
        "out5 = ivec4(0, 0, 0, 0);",
        "out6 = ivec4(0, 0, 0, 0);",
        "out7 = vec4(1.0f, 0.0f, 0.0f, 0.5f);",
    };

    // Generate the shaders, [0] for first draw and [1] for second.
    std::stringstream fsStr[2];
    for (unsigned int index = 0; index < 2; ++index)
    {
        fsStr[index] << R"(#version 300 es
precision highp float;
)";

        for (GLint drawBuffer = 0; drawBuffer < maxDrawBuffers; ++drawBuffer)
        {
            fsStr[index] << kDecl[drawBuffer] << "\n";
        }

        fsStr[index] << R"(void main()
{
)";

        const char *const *assign = index == 0 ? kAssign1 : kAssign2;
        for (GLint drawBuffer = 0; drawBuffer < maxDrawBuffers; ++drawBuffer)
        {
            fsStr[index] << assign[drawBuffer] << "\n";
        }

        fsStr[index] << "}\n";
    }

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into
    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);

    GLTexture textureMS[kImplMaxDrawBuffers];
    GLRenderbuffer renderbufferMS[kImplMaxDrawBuffers];
    for (GLint drawBuffer = 0; drawBuffer < maxDrawBuffers; ++drawBuffer)
    {
        createAndAttachColorAttachment(useRenderbuffer, kSize, GL_COLOR_ATTACHMENT0 + drawBuffer,
                                       &kGLType[drawBuffer], &textureMS[drawBuffer],
                                       &renderbufferMS[drawBuffer]);
    }
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Setup programs
    ANGLE_GL_PROGRAM(drawProg, essl3_shaders::vs::Simple(), fsStr[0].str().c_str());
    ANGLE_GL_PROGRAM(blendProg, essl3_shaders::vs::Simple(), fsStr[1].str().c_str());

    constexpr GLenum kDrawBuffers[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7};
    glDrawBuffers(maxDrawBuffers, kDrawBuffers);
    ASSERT_GL_NO_ERROR();

    // Draw into the multisampled color buffers.
    glUseProgram(drawProg);
    drawQuad(drawProg, essl3_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy from one of them.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend color buffers.
    glUseProgram(blendProg);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(blendProg, essl3_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify texture colors.
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    const GLColor kExpected0(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected0, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected0, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected0, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected0, 1);

    glReadBuffer(GL_COLOR_ATTACHMENT3);
    const GLColor kExpected3(0, 127, 127, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected3, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected3, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected3, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected3, 1);

    if (maxDrawBuffers > 7)
    {
        glReadBuffer(GL_COLOR_ATTACHMENT7);
        const GLColor kExpected7(127, 0, 127, 191);
        EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected7, 1);
        EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected7, 1);
        EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected7, 1);
        EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected7, 1);
    }

    // For completeness, verify that the texture used as copy target is red.
    const GLColor kExpectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, kExpectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();
}

// Draw, copy, then blend with 8 mixed format attachments.  The copy will make sure an implicit
// resolve happens.  Regardless, the following draw should retain the data written by the first draw
// command.
TEST_P(MultisampledRenderToTextureES31Test, DrawCopyThenBlendAllAttachmentsMixed)
{
    drawCopyThenBlendAllAttachmentsMixed(false);
}

// Same as DrawCopyThenBlendAllAttachmentsMixed but with renderbuffers.
TEST_P(MultisampledRenderToTextureES31Test, RenderbufferDrawCopyThenBlendAllAttachmentsMixed)
{
    drawCopyThenBlendAllAttachmentsMixed(true);
}

void MultisampledRenderToTextureES3Test::renderbufferUnresolveColorAndDepthStencilThenTwoColors(
    bool withDepth,
    bool withStencil)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    // http://anglebug.com/5096
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    GLFramebuffer fboColorAndDepthStencil;
    glBindFramebuffer(GL_FRAMEBUFFER, fboColorAndDepthStencil);

    // Create framebuffer to draw into, with both color and depth/stencil attachments.
    GLTexture color1;
    glBindTexture(GL_TEXTURE_2D, color1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         color1, 0, 4);

    GLenum depthStencilFormat = GL_DEPTH24_STENCIL8;
    GLenum depthStencilTarget = GL_DEPTH_STENCIL_ATTACHMENT;

    ASSERT_TRUE(withDepth || withStencil);
    if (withDepth && !withStencil)
    {
        depthStencilFormat = GL_DEPTH_COMPONENT24;
        depthStencilTarget = GL_DEPTH_ATTACHMENT;
    }
    if (!withDepth && withStencil)
    {
        depthStencilFormat = GL_STENCIL_INDEX8;
        depthStencilTarget = GL_STENCIL_ATTACHMENT;
    }

    GLRenderbuffer depthStencil;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, depthStencilFormat, kSize, kSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, depthStencilTarget, GL_RENDERBUFFER, depthStencil);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear depth/stencil
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    if (withDepth)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    // If stencil is not cleared to 0x55, rendering would fail.
    if (withStencil)
    {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0x55, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0xFF);
    }

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.25f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Draw again into the framebuffer, this time blending.  This tests that both the color and
    // depth/stencil data are preserved after the resolve incurred by the copy above.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);

    // Now create a framebuffer with two color attachments and do something similar.  This makes
    // sure that the fact that both these framebuffers have 2 attachments does not cause confusion,
    // for example by having the unresolve shader generated for the first framebuffer used for the
    // second framebuffer.
    GLFramebuffer fboTwoColors;
    glBindFramebuffer(GL_FRAMEBUFFER, fboTwoColors);

    GLTexture color2;
    glBindTexture(GL_TEXTURE_2D, color2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         color2, 0, 4);

    GLTexture color3;
    glBindTexture(GL_TEXTURE_2D, color3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         color3, 0, 4);

    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    // Setup program
    constexpr bool kBuffersEnabled[8] = {true, true};

    GLuint drawColorMRT;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColorMRT);
    glUseProgram(drawColorMRT);
    GLint colorUniformLocationMRT =
        glGetUniformLocation(drawColorMRT, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocationMRT, -1);

    // Draw blue
    glUniform4f(colorUniformLocationMRT, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColorMRT, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Copy into texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);

    // Blend.
    glUniform4f(colorUniformLocationMRT, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColorMRT, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now cyan
    const GLColor kExpected2(0, 127, 127, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected2, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected2, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected2, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected2, 1);

    // For completeness, verify that the texture used as copy target is blue.
    const GLColor expectedCopyResult2(0, 0, 255, 255);
    verifyResults(texture, expectedCopyResult2, kSize, 0, 0, kSize, kSize);
}

// Draw, copy, then blend once on a framebuffer with color and depth attachments, and once with two
// color attachments.  Tests that unresolve is done correctly on two framebuffers with the same
// number of attachments, but differing in depth being there.  Note that this test doesn't apply to
// depth/stencil textures as they are explicitly autoinvalidated between render passes.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferUnresolveColorAndDepthThenTwoColors)
{
    renderbufferUnresolveColorAndDepthStencilThenTwoColors(true, false);
}

// Similar to RenderbufferUnresolveColorAndDepthThenTwoColors, but with stencil.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferUnresolveColorAndStencilThenTwoColors)
{
    renderbufferUnresolveColorAndDepthStencilThenTwoColors(false, true);
}

// Similar to RenderbufferUnresolveColorAndDepthThenTwoColors, but with depth and stencil.
TEST_P(MultisampledRenderToTextureES3Test, RenderbufferUnresolveColorAndDepthStencilThenTwoColors)
{
    renderbufferUnresolveColorAndDepthStencilThenTwoColors(true, true);
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3_AND_ES31(MultisampledRenderToTextureTest);
ANGLE_INSTANTIATE_TEST_ES3(MultisampledRenderToTextureES3Test);
ANGLE_INSTANTIATE_TEST_ES31(MultisampledRenderToTextureES31Test);
}  // namespace
