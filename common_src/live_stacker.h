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
 * Two alignment methods are available (see AlignmentMethod).  Rotation
 * correction is not performed; sub-pixel translation is applied using
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
 *   for (auto *frame : frames)
 *       stacker.addImage(frame);
 *   preview_image *result = stacker.currentStack(); // caller owns result
 * @endcode
 */
class LiveStacker {
public:
	/// Alignment algorithm used when adding frames.
	enum AlignmentMethod {
		ALIGN_NCC,       ///< Two-level pyramid normalised cross-correlation.
		ALIGN_CENTROIDS  ///< Multi-star centroid matching (Siril / N.I.N.A. style) — default.
	};

	/// Interpolation method used when resampling frames during accumulation.
	enum InterpolationMethod {
		INTERP_NEAREST,  ///< Nearest-neighbour — fastest, no sub-pixel smoothing.
		INTERP_BILINEAR, ///< Bilinear interpolation — fast, smooth (default).
		INTERP_BICUBIC   ///< Catmull-Rom bicubic — sharper, slightly slower.
	};

	LiveStacker();
	~LiveStacker() = default;

	/// Select the alignment algorithm.  Takes effect from the next resetStack().
	void setAlignmentMethod(AlignmentMethod method) { m_alignment_method = method; }
	/// Returns the currently selected alignment algorithm.
	AlignmentMethod alignmentMethod() const { return m_alignment_method; }

	/// Select the sub-pixel interpolation method used during accumulation.
	void setInterpolationMethod(InterpolationMethod method) { m_interp_method = method; }
	/// Returns the currently selected interpolation method.
	InterpolationMethod interpolationMethod() const { return m_interp_method; }

	/// Start a new stack — identical to resetStack().
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
	/// Build a downsampled, mean-subtracted luminance map at 1/@p ds resolution.
	std::vector<float> buildLuminanceMap(preview_image *image, int ds) const;

	/**
	 * Two-level pyramid shift estimation.
	 *
	 * Level 1 (coarse): search ±SEARCH_PX in original coords using the
	 * DS_COARSE-downsampled map.  Fast because the map is tiny.
	 *
	 * Level 2 (fine): refine ±DS_COARSE pixels around the coarse result
	 * using the DS_FINE-downsampled map.  Then applies parabolic sub-pixel
	 * refinement, returning a fractional shift in ORIGINAL pixel coordinates.
	 */
	void findShift(double &shift_x, double &shift_y,
	               const std::vector<float> &cur_coarse,
	               const std::vector<float> &cur_fine) const;

	/// Returns the NCC score for shifting @p cur by (dx, dy) relative to @p ref.
	static double nccScore(const std::vector<float> &ref,
	                       const std::vector<float> &cur,
	                       int dW, int dH, int dx, int dy);

	/// Accumulate @p image into the internal buffer shifted by (dx, dy)
	/// using the selected interpolation method (bilinear or bicubic).
	void accumulate(preview_image *image, double dx, double dy);

	// ---- centroid-based alignment helpers ---------------------------------

	/**
	 * @brief Detect stars in @p image and return their sub-pixel centroids.
	 *
	 * Works on a box-downsampled copy at 1/DS_STAR resolution to reduce noise
	 * and speed up detection.  Candidate peaks are local maxima above
	 * (background + STAR_SIGMA * noise), refined with an intensity-weighted
	 * centroid in a STAR_WIN×STAR_WIN window.  The top STAR_MAX_COUNT peaks
	 * by integrated flux are returned with coordinates converted back to
	 * original-pixel space.
	 */
	std::vector<StarCentroid> detectStars(preview_image *image) const;

	/**
	 * @brief Estimate the translation shift using matched star centroids.
	 *
	 * Each star in @p cur_stars is matched to the nearest star in the stored
	 * reference list (m_ref_stars) within STAR_MATCH_RADIUS original pixels.
	 * The shift is the median (dx, dy) over all matched pairs.  Requires at
	 * least STAR_MIN_MATCHES pairs; returns false (and leaves shift at 0)
	 * when too few stars are matched.
	 */
	bool findShiftByCentroids(double &shift_x, double &shift_y,
	                          const std::vector<StarCentroid> &cur_stars) const;

	std::vector<double>      m_acc;           ///< channels * height * width
	std::vector<float>       m_ref_coarse;    ///< DS_COARSE-downsampled luminance of frame 0
	std::vector<float>       m_ref_fine;      ///< DS_FINE-downsampled  luminance of frame 0
	std::vector<StarCentroid> m_ref_stars;    ///< Stars detected in frame 0 for centroid alignment
	AlignmentMethod          m_alignment_method;
	InterpolationMethod      m_interp_method;
	int  m_width;
	int  m_height;
	int  m_channels;
	int  m_pix_format;
	int  m_frame_count;
};

#endif // LIVE_STACKER_H
