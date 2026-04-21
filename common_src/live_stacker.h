// Copyright (c) 2026 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LIVE_STACKER_H
#define LIVE_STACKER_H

#include <vector>
#include <imagepreview.h>

/**
 * @brief Sub-pixel star centroid, used for centroid-based alignment.
 */
struct StarCentroid {
	float x;    ///< Sub-pixel column in ORIGINAL pixel coordinates.
	float y;    ///< Sub-pixel row    in ORIGINAL pixel coordinates.
	float flux; ///< Integrated background-subtracted brightness in the detection window.
};

/**
 * @brief LiveStacker accumulates frames in a double-precision float buffer,
 *        aligning each new frame to the reference (first) frame.
 *
 * Two alignment methods are available (see AlignmentMethod).  Optional
 * rotation correction (field rotation, meridian flip) is estimated from the
 * same star matches and applied together with sub-pixel translation using
 * bicubic (Catmull-Rom) interpolation.
 * The output stack is a newly allocated @c preview_image using PIX_FMT_F32
 * (mono) or PIX_FMT_RGBF (colour) whose raw data contains the per-pixel
 * mean value across all accumulated frames.
 *
 * Typical usage:
 * @code
 *   LiveStacker stacker;
 *   stacker.setAlignmentMethod(LiveStacker::ALIGN_CENTROIDS);
 *   stacker.startStack();
 *   for (auto *frame : frames) {
 *       stacker.addImage(frame);
 *   }
 *   preview_image *result = stacker.currentStack(); // caller owns result
 * @endcode
 */
class LiveStacker {
public:
	/// Alignment algorithm used when adding frames.
	enum AlignmentMethod {
		ALIGN_CENTROIDS,         ///< Multi-star centroid matching (nearest-neighbour + median, brute-force O(N×M)).
		ALIGN_HOUGH,             ///< Hough-style translation-histogram voting over all star pairs.
		ALIGN_KD_TREE,           ///< k-d tree NN matching: O(N log M), no radius constraint, robust median shift.
		ALIGN_KD_TREE_ROTATION   ///< k-d tree + full rigid-body (rotation + translation) via Kabsch/RANSAC — default.
	};

	/// Interpolation method used when resampling frames during accumulation.
	enum InterpolationMethod {
		INTERP_NEAREST,  ///< Nearest-neighbour — fastest, no sub-pixel smoothing.
		INTERP_BILINEAR, ///< Bilinear interpolation — fast, smooth (default).
		INTERP_BICUBIC   ///< Catmull-Rom bicubic — sharper, slightly slower.
	};

	LiveStacker();
	~LiveStacker() = default;

	/// Takes effect from the next resetStack().
	void setAlignmentMethod(AlignmentMethod method) { m_alignment_method = method; }
	AlignmentMethod alignmentMethod() const { return m_alignment_method; }

	void setInterpolationMethod(InterpolationMethod method) { m_interp_method = method; }
	InterpolationMethod interpolationMethod() const { return m_interp_method; }

	/// Start a new stack — alias to resetStack().
	void startStack();

	/// Reset the accumulator and forget the reference frame so the next
	/// call to addImage() establishes a new reference.
	void resetStack();

	/**
	 * @brief Add @p image to the stack.
	 *
	 * The first frame added after a reset becomes the alignment reference.
	 * Every subsequent frame is translated to align with the reference before
	 * being accumulated.  Alignment uses the method set by setAlignmentMethod().
	 *
	 * @param image  Source frame.  The stacker does NOT take ownership; the
	 *               pointer must remain valid for the duration of the call.
	 * @return @c true on success; @c false when @p image is nullptr, has no
	 *         raw data, or has a different size / pixel format from the
	 *         reference frame already stored in the stack.
	 */
	bool addImage(preview_image *image);

	/// Number of frames accumulated since the last reset.
	int stackCount() const { return m_frame_count; }

	/// @c true once at least one frame has been accumulated.
	bool isStarted() const { return m_frame_count > 0; }

	/**
	 * @brief Return the current stack as a new @c preview_image.
	 *
	 * The pixel values are the per-pixel mean over all accumulated frames.
	 * The caller owns the returned object and is responsible for deleting it.
	 *
	 * @return Newly allocated preview_image, or @c nullptr if no frames have
	 *         been added yet.
	 */
	preview_image *currentStack() const;

private:
	std::vector<float> buildLuminanceMap(preview_image *image, int ds) const;
	void accumulate(preview_image *image, double dx, double dy, double theta = 0.0);
	void accumulateNearest (preview_image *image, double dx, double dy, double theta);
	void accumulateBilinear(preview_image *image, double dx, double dy, double theta);
	void accumulateBicubic (preview_image *image, double dx, double dy, double theta);

	// ---- centroid-based alignment helpers ---------------------------------
	std::vector<StarCentroid> detectStars(preview_image *image) const;
	bool findShiftByCentroids(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const;
	bool findShiftByKdTree(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const;
	bool findShiftByHough(double &shift_x, double &shift_y, const std::vector<StarCentroid> &cur_stars) const;
	bool findRotationAndShift(double &shift_x, double &shift_y, double &theta, const std::vector<StarCentroid> &cur_stars) const;
	int tryAlign(const std::vector<StarCentroid> &stars, double &out_theta, double &out_sx, double &out_sy) const;

	/// Matched star pair used internally by tryAlign and findRotationAndShift.
	struct AlignPair {
		float rx, ry, cx_s, cy_s;
	};

	std::vector<double> m_acc;                ///< channels * height * width
	std::vector<StarCentroid> m_ref_stars;    ///< Stars detected in frame 0 for centroid alignment
	AlignmentMethod m_alignment_method;
	InterpolationMethod m_interp_method;
	int  m_width;
	int  m_height;
	int  m_channels;
	int  m_pix_format;
	int  m_frame_count;
};

#endif // LIVE_STACKER_H
