/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrTextureProducer_DEFINED
#define GrTextureProducer_DEFINED

#include "include/core/SkImageInfo.h"
#include "include/private/GrResourceKey.h"
#include "include/private/SkNoncopyable.h"
#include "src/gpu/GrImageInfo.h"
#include "src/gpu/GrSamplerState.h"

class GrFragmentProcessor;
class GrRecordingContext;
class GrTexture;
class GrTextureProxy;
class SkColorSpace;
class SkMatrix;
struct SkRect;

/**
 * Different GPUs and API extensions have different requirements with respect to what texture
 * sampling parameters may be used with textures of various types. This class facilitates making
 * texture compatible with a given GrSamplerState. There are two immediate subclasses defined
 * below. One is a base class for sources that are inherently texture-backed (e.g. a texture-backed
 * SkImage). It supports subsetting the original texture. The other is for use cases where the
 * source can generate a texture that represents some content (e.g. cpu pixels, SkPicture, ...).
 */
class GrTextureProducer : public SkNoncopyable {
public:
    virtual ~GrTextureProducer() {}

    enum FilterConstraint {
        kYes_FilterConstraint,
        kNo_FilterConstraint,
    };

    /**
     * Helper for creating a fragment processor to sample the texture with a given filtering mode.
     * It attempts to avoid making texture copies or using domains whenever possible.
     *
     * @param textureMatrix                    Matrix used to access the texture. It is applied to
     *                                         the local coords. The post-transformed coords should
     *                                         be in texel units (rather than normalized) with
     *                                         respect to this Producer's bounds (width()/height()).
     * @param constraintRect                   A rect that represents the area of the texture to be
     *                                         sampled. It must be contained in the Producer's
     *                                         bounds as defined by width()/height().
     * @param filterConstriant                 Indicates whether filtering is limited to
     *                                         constraintRect.
     * @param coordsLimitedToConstraintRect    Is it known that textureMatrix*localCoords is bound
     *                                         by the portion of the texture indicated by
     *                                         constraintRect (without consideration of filter
     *                                         width, just the raw coords).
     * @param filterOrNullForBicubic           If non-null indicates the filter mode. If null means
     *                                         use bicubic filtering.
     **/
    virtual std::unique_ptr<GrFragmentProcessor> createFragmentProcessor(
            const SkMatrix& textureMatrix,
            const SkRect& constraintRect,
            FilterConstraint filterConstraint,
            bool coordsLimitedToConstraintRect,
            const GrSamplerState::Filter* filterOrNullForBicubic) = 0;

    /**
     * Returns a texture view, possibly with MIP maps. The request for MIP maps may not be honored
     * base on caps, format, and whether the texture is 1x1. A non-MIP mapped request may still
     * receive a MIP mapped texture (if that is what is available in the cache).
     */
    GrSurfaceProxyView view(GrMipMapped);

    /** Helper version of above that determines MIP mapping requirement from Filter. */
    GrSurfaceProxyView view(GrSamplerState::Filter);

    int width() const { return fImageInfo.width(); }
    int height() const { return fImageInfo.height(); }
    SkISize dimensions() const { return fImageInfo.dimensions(); }
    GrColorType colorType() const { return fImageInfo.colorType(); }
    SkAlphaType alphaType() const { return fImageInfo.alphaType(); }
    SkColorSpace* colorSpace() const { return fImageInfo.colorSpace(); }
    bool isAlphaOnly() const { return GrColorTypeIsAlphaOnly(fImageInfo.colorType()); }
    bool domainNeedsDecal() const { return fDomainNeedsDecal; }
    // If the "texture" samples multiple images that have different resolutions (e.g. YUV420)
    virtual bool hasMixedResolutions() const { return false; }

protected:
    friend class GrTextureProducer_TestAccess;

    GrTextureProducer(GrRecordingContext* context,
                      const GrImageInfo& imageInfo,
                      bool domainNeedsDecal)
            : fContext(context), fImageInfo(imageInfo), fDomainNeedsDecal(domainNeedsDecal) {}

    enum DomainMode {
        kNoDomain_DomainMode,
        kDomain_DomainMode,
        kTightCopy_DomainMode
    };

    static DomainMode DetermineDomainMode(const SkRect& constraintRect,
                                          FilterConstraint filterConstraint,
                                          bool coordsLimitedToConstraintRect,
                                          GrSurfaceProxy*,
                                          const GrSamplerState::Filter* filterModeOrNullForBicubic,
                                          SkRect* domainRect);

    std::unique_ptr<GrFragmentProcessor> createFragmentProcessorForDomainAndFilter(
            GrSurfaceProxyView view,
            const SkMatrix& textureMatrix,
            DomainMode,
            const SkRect& domain,
            const GrSamplerState::Filter* filterOrNullForBicubic);

    GrRecordingContext* context() const { return fContext; }

private:
    virtual GrSurfaceProxyView onView(GrMipMapped) = 0;

    GrRecordingContext* fContext;
    const GrImageInfo fImageInfo;
    // If true, any domain effect uses kDecal instead of kClamp, and sampler filter uses
    // kClampToBorder instead of kClamp.
    const bool  fDomainNeedsDecal;

    typedef SkNoncopyable INHERITED;
};

#endif
