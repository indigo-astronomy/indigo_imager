#include "image_inspector.h"
#include "imagepreview.h"
#include "snr_calculator.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <future>

ImageInspector::ImageInspector() {}
ImageInspector::~ImageInspector() {}

// file-local helper types
struct Candidate { double x; double y; double hfd; double star_radius; double snr; double moment_m20; double moment_m02; double moment_m11; double eccentricity; };
struct UniqueCand { double x; double y; double snr; double hfd; double eccentricity; int cell; };

struct CellResult {
    int cell_index;
    double hfd = 0.0;
    int detected = 0;
    int used = 0;
    int rejected = 0;
    std::vector<std::tuple<double,double,double,double>> per_cell_used; // x,y,snr,star_radius
    std::vector<UniqueCand> unique_candidates; // for recomputing detected counts
    std::vector<QPointF> rejected_points;
    double eccentricity = 0.0;
    double major_angle_deg = 0.0;
};

static void compute_mean_stddev_in_area(const preview_image &img, int sx0, int sy0, int sx1, int sy1,
                                        double &out_mean, double &out_sd) {
    double sum = 0.0;
    double sumsq = 0.0;
    int count = 0;
    for (int yy = sy0; yy <= sy1; ++yy) {
        for (int xx = sx0; xx <= sx1; ++xx) {
            double v = 0;
            double g = 0;
            double b = 0;
            img.pixel_value(xx, yy, v, g, b);
            sum += v;
            sumsq += v * v;
            ++count;
        }
    }
    out_mean = (count>0) ? (sum / count) : 0.0;
    double var = (count>0) ? (sumsq / count - out_mean * out_mean) : 0.0;
    out_sd = var > 0.0 ? std::sqrt(var) : 0.0;
}

// Deduplicate raw candidates within a cell: keep only those with snr > threshold and with
// duplicates merged by keeping the highest-SNR entry inside duplicate_radius.
static std::vector<Candidate> deduplicate_within_cell(const std::vector<Candidate> &candidates,
                                                     double snr_threshold, double duplicate_radius) {
    std::vector<Candidate> unique;
    for (const Candidate &c : candidates) {
        if (!(c.snr > snr_threshold && c.hfd > 0)) continue;
        bool replaced = false;
        for (Candidate &u : unique) {
            double dx = c.x - u.x, dy = c.y - u.y;
            double dist = std::sqrt(dx*dx + dy*dy);
            if (dist <= duplicate_radius) {
                if (c.snr > u.snr) {
                    u = c;
                }
                replaced = true;
                break;
            }
        }
        if (!replaced) unique.push_back(c);
    }
    return unique;
}

static double sigma_clip_hfd(const std::vector<Candidate> &unique, std::vector<int> &used_indices) {
    used_indices.clear();
    if (unique.empty()) return 0.0;
    std::vector<std::pair<double,int>> hv;
    for (size_t i = 0; i < unique.size(); ++i) {
        hv.emplace_back(unique[i].hfd, static_cast<int>(i));
    }
    std::vector<std::pair<double,int>> v = hv;
    for (int iter = 0; iter < 3; ++iter) {
        if (v.empty()) break;
        double sum = 0.0;
        double sumsq = 0.0;
        for (auto &p : v) {
            sum += p.first;
            sumsq += p.first * p.first;
        }
        double mean = sum / v.size();
        double var = sumsq / v.size() - mean * mean;
        double sd = var > 0 ? std::sqrt(var) : 0.0;
        std::vector<std::pair<double,int>> nv;
        for (auto &p : v) {
            if (std::fabs(p.first - mean) <= 2.0 * sd) nv.push_back(p);
        }
        if (nv.size() == v.size()) break;
        if (nv.empty()) break;
        v.swap(nv);
    }
    for (auto &p : v) {
        used_indices.push_back(p.second);
    }
    double sum = 0.0;
    for (auto &p : v) {
        sum += p.first;
    }
    double avg = v.empty() ? 0.0 : sum / v.size();
    return avg;
}

static CellResult analyze_cell(const preview_image &img, int width, int height, int gx, int gy,
                              int cx, int cy, int cell_w, int cell_h, double INSPECTION_SNR_THRESHOLD) {
    CellResult cr;
    cr.cell_index = cy * gx + cx;

    int x0 = cx * cell_w;
    int y0 = cy * cell_h;
    int x1 = (cx == gx-1) ? width-1 : x0 + cell_w - 1;
    int y1 = (cy == gy-1) ? height-1 : y0 + cell_h - 1;
    const int MARGIN_SEARCH = 8;
    const int MARGIN_CENTROID = 8;
    int sx0 = std::max(0, x0 - MARGIN_SEARCH);
    int sy0 = std::max(0, y0 - MARGIN_SEARCH);
    int sx1 = std::min(width - 1, x1 + MARGIN_SEARCH);
    int sy1 = std::min(height - 1, y1 + MARGIN_SEARCH);

    // compute mean/stddev in search area
    double cell_mean = 0.0, cell_sd = 0.0;
    compute_mean_stddev_in_area(img, sx0, sy0, sx1, sy1, cell_mean, cell_sd);
    double peak_threshold = cell_mean + 1.5 * cell_sd;

    std::vector<Candidate> candidates;
    for (int yy = sy0; yy <= sy1; ++yy) {
        for (int xx = sx0; xx <= sx1; ++xx) {
            double center_val=0, gv=0, bv=0; img.pixel_value(xx, yy, center_val, gv, bv);
            if (center_val <= peak_threshold) continue;
            bool is_local_max = true;
            for (int ny = yy - 1; ny <= yy + 1; ++ny) {
                for (int nx = xx - 1; nx <= xx + 1; ++nx) {
                    if (nx == xx && ny == yy) continue;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    double nval = 0, ng = 0, nb = 0;
                    img.pixel_value(nx, ny, nval, ng, nb);
                    if (nval > center_val) {
                        is_local_max = false;
                        break;
                    }
                }
                if (!is_local_max) {
                    break;
                }
            }
            if (!is_local_max) continue;
            SNRResult r = calculateSNR(reinterpret_cast<const uint8_t*>(img.m_raw_data), width, height, img.m_pix_format, xx, yy);
                if (r.valid && !r.is_saturated) {
                int cx_i = static_cast<int>(std::round(r.star_x));
                int cy_i = static_cast<int>(std::round(r.star_y));
                if (cx_i >= x0 - MARGIN_CENTROID && cx_i <= x1 + MARGIN_CENTROID && cy_i >= y0 - MARGIN_CENTROID && cy_i <= y1 + MARGIN_CENTROID) {
                    candidates.push_back({r.star_x, r.star_y, r.hfd, r.star_radius, r.snr, r.moment_m20, r.moment_m02, r.moment_m11, r.eccentricity});
                }
            }
        }
    }

    // deduplicate within cell (keep highest-SNR duplicates)
    const double DUPLICATE_RADIUS = 5.0;
    std::vector<Candidate> unique = deduplicate_within_cell(candidates, INSPECTION_SNR_THRESHOLD, DUPLICATE_RADIUS);

    const int MIN_STARS_PER_CELL = 6;
    const double FALLBACK_SNR = 6.0;
    if (unique.size() < static_cast<size_t>(MIN_STARS_PER_CELL)) {
        int fallback_step = 4;
        std::vector<Candidate> fallback_found;
        for (int yy = y0; yy <= y1; yy += fallback_step) {
            for (int xx = x0; xx <= x1; xx += fallback_step) {
                SNRResult r = calculateSNR(reinterpret_cast<const uint8_t*>(img.m_raw_data), width, height, img.m_pix_format, xx, yy);
                if (!r.valid || r.is_saturated) continue;
                if (!(r.snr > FALLBACK_SNR && r.hfd > 0)) continue;
                int cx_i = static_cast<int>(std::round(r.star_x));
                int cy_i = static_cast<int>(std::round(r.star_y));
                const int MARGIN_CENTROID_FALLBACK = 12;
                if (cx_i < x0 - MARGIN_CENTROID_FALLBACK || cx_i > x1 + MARGIN_CENTROID_FALLBACK || cy_i < y0 - MARGIN_CENTROID_FALLBACK || cy_i > y1 + MARGIN_CENTROID_FALLBACK) continue;
                fallback_found.push_back({r.star_x, r.star_y, r.hfd, r.star_radius, r.snr, r.moment_m20, r.moment_m02, r.moment_m11, r.eccentricity});
            }
        }
        if (!fallback_found.empty()) {
            // merge existing unique and fallback and re-deduplicate using fallback threshold
            std::vector<Candidate> merged = unique;
            merged.insert(merged.end(), fallback_found.begin(), fallback_found.end());
            unique = deduplicate_within_cell(merged, FALLBACK_SNR, DUPLICATE_RADIUS);
        }
    }

    // sigma-clip HFD values
    std::vector<int> used_indices;
    double avg = sigma_clip_hfd(unique, used_indices);
    int used_after = static_cast<int>(used_indices.size());
    int rejected = static_cast<int>(unique.size()) - used_after;

    for (size_t i=0;i<unique.size();++i) {
        bool is_used = false;
        for (int idx : used_indices) {
            if (static_cast<size_t>(idx) == i) {
                is_used = true;
                break;
            }
        }
        QPointF centerScene(unique[i].x, unique[i].y);
        double rscene = unique[i].star_radius;
        if (is_used) {
            cr.per_cell_used.emplace_back(unique[i].x, unique[i].y, unique[i].snr, rscene);
        } else {
            cr.rejected_points.push_back(centerScene);
        }
        cr.unique_candidates.push_back({unique[i].x, unique[i].y, unique[i].snr, unique[i].hfd, unique[i].eccentricity, cy * gx + cx});
    }
    // Aggregate weighted per-star normalized central moments (provided by SNRResult)
    double m20 = 0.0, m02 = 0.0, m11 = 0.0, total_w = 0.0;
    double ecc_sum = 0.0;
	int ecc_count = 0;
	printf("Cell (%d,%d) detected=%zu used=%d rejected=%d avg_hfd=%.2f\n", cx, cy, unique.size(), used_after, rejected, avg);

    for (int idx : used_indices) {
        if (idx < 0 || static_cast<size_t>(idx) >= unique.size()) continue;
        const Candidate &uc = unique[idx];
        double w = std::max(1.0, uc.snr);
        m20 += w * uc.moment_m20;
        m02 += w * uc.moment_m02;
        m11 += w * uc.moment_m11;
        ecc_sum += w * uc.eccentricity;
        total_w += w;
		++ecc_count;
		printf("  Used star[%d]: x=%.2f y=%.2f snr=%.2f hfd=%.2f ecc=%.3f\n", ecc_count, uc.x, uc.y, uc.snr, uc.hfd, uc.eccentricity);
    }

    if (total_w > 0.0) {
		m20 /= total_w;
		m02 /= total_w;
		m11 /= total_w;

		double trace = m20 + m02;
        double det = m20 * m02 - m11 * m11;
        double discriminant = trace * trace - 4.0 * det;
        if (discriminant < 0) discriminant = 0;
        double lambda1 = (trace + std::sqrt(discriminant)) / 2.0;
        double lambda2 = (trace - std::sqrt(discriminant)) / 2.0;
        // keep major axis angle calculation as before
        double ang_rad = 0.5 * std::atan2(2.0 * m11, m02 - m20);
        double ang_deg = ang_rad * 180.0 / M_PI;
        while (ang_deg < 0) ang_deg += 180.0;
        while (ang_deg >= 180.0) ang_deg -= 180.0;
        // weighted average eccentricity from per-star SNRResult.eccentricity
        double ecc_avg = (total_w > 0.0) ? (ecc_sum / total_w) : 0.0;
		printf("  Cell major axis angle=%.2f deg eccentricity=%.3f\n", ang_deg, ecc_avg);
        cr.eccentricity = ecc_avg;
        cr.major_angle_deg = ang_deg;
    } else {
        cr.eccentricity = 0.0;
        cr.major_angle_deg = 0.0;
    }
    cr.hfd = avg;
    size_t nd = unique.size();
    cr.detected = static_cast<int>(std::min(nd, static_cast<size_t>(9999)));
    cr.used = std::min(used_after, 9999);
    cr.rejected = std::min(rejected, 9999);
    return cr;
}

InspectionResult ImageInspector::inspect(const preview_image &img, int gx, int gy, double INSPECTION_SNR_THRESHOLD) {
    InspectionResult res;
    // validate
    if (img.m_raw_data == nullptr) return res;
    int width = img.width();
    int height = img.height();
    if (width <= 0 || height <= 0) return res;

    int cell_w = width / gx;
    int cell_h = height / gy;
    std::vector<double> cell_hfd(gx * gy, 0.0);
    std::vector<int> cell_detected(gx * gy, 0);
    std::vector<int> cell_used(gx * gy, 0);
    std::vector<int> cell_rejected(gx * gy, 0);

    // mark target cells (4 corners, 4 mid-edges, center)
    std::vector<bool> isTarget(gx * gy, false);
    auto mark = [&](int tx, int ty) {
        if (tx >= 0 && tx < gx && ty >= 0 && ty < gy) {
            isTarget[ty * gx + tx] = true;
        }
    };
    mark(0,0); mark(gx-1,0); mark(gx-1,gy-1); mark(0,gy-1);
    mark(gx/2, 0); mark(gx-1, gy/2); mark(gx/2, gy-1); mark(0, gy/2); mark(gx/2, gy/2);

    // launch per-target-cell analysis in parallel
    std::vector<std::future<CellResult>> futures(gx * gy);
    for (int cy = 0; cy < gy; ++cy) {
        for (int cx = 0; cx < gx; ++cx) {
            int idx = cy * gx + cx;
            if (!isTarget[idx]) continue;
            futures[idx] = std::async(std::launch::async, analyze_cell, std::cref(img), width, height, gx, gy, cx, cy, cell_w, cell_h, INSPECTION_SNR_THRESHOLD);
        }
    }

    // collect results
    std::vector<std::vector<std::tuple<double,double,double,double>>> per_cell_used(gx * gy);
    std::vector<UniqueCand> all_unique_candidates;
    std::vector<QPointF> inspection_rejected_points;
    std::vector<double> per_cell_ecc(gx * gy, 0.0);
    std::vector<double> per_cell_angle(gx * gy, 0.0);
    for (int cy = 0; cy < gy; ++cy) {
        for (int cx = 0; cx < gx; ++cx) {
            int idx = cy * gx + cx;
            if (!isTarget[idx]) continue;
            CellResult cr = futures[idx].get();
            cell_hfd[idx] = cr.hfd;
            cell_detected[idx] = cr.detected;
            cell_used[idx] = cr.used;
            cell_rejected[idx] = cr.rejected;
            per_cell_used[idx] = std::move(cr.per_cell_used);
            per_cell_ecc[idx] = cr.eccentricity;
            per_cell_angle[idx] = cr.major_angle_deg;
            // append unique candidates and rejected points
            for (auto &u : cr.unique_candidates) all_unique_candidates.push_back(u);
            for (auto &rp : cr.rejected_points) inspection_rejected_points.push_back(rp);
        }
    }

    // global dedup
    struct GlobalEntry { double x; double y; double snr; double star_radius; int cell; };
    std::vector<GlobalEntry> global_used;
    const double GLOBAL_DUP_RADIUS = 5.0;
    for (int ci = 0; ci < gx * gy; ++ci) {
        for (auto &t : per_cell_used[ci]) {
            double x = std::get<0>(t); double y = std::get<1>(t); double snr = std::get<2>(t); double sr = std::get<3>(t);
            bool merged = false;
            for (auto &g : global_used) {
                double dx = g.x - x; double dy = g.y - y;
                if (std::sqrt(dx*dx + dy*dy) <= GLOBAL_DUP_RADIUS) {
                    if (snr > g.snr) {
                        g.x = x;
                        g.y = y;
                        g.snr = snr;
                        g.star_radius = sr;
                    }
                    merged = true;
                    break;
                }
            }
            if (!merged) global_used.push_back({x,y,snr,sr,ci});
        }
    }

    // rebuild used points and used counts per cell
    std::fill(cell_used.begin(), cell_used.end(), 0);
    std::vector<QPointF> inspection_used_points;
    std::vector<double> inspection_used_radii;
    for (const auto &g : global_used) {
        inspection_used_points.push_back(QPointF(g.x, g.y));
        inspection_used_radii.push_back(g.star_radius);
        int cell_index = g.cell;
        if (cell_index>=0 && cell_index < gx*gy) {
            cell_used[cell_index]++;
        }
    }

    // recompute detected per owner cell
    std::vector<int> cell_detected_final(gx * gy, 0);
    for (const auto &u : all_unique_candidates) {
        int owner = u.cell;
        if (owner<0 || owner>=gx*gy) continue;
        cell_detected_final[owner]++;
    }
    for (int i=0;i<gx*gy;++i) { cell_detected[i] = cell_detected_final[i]; cell_rejected[i] = std::max(0, cell_detected[i] - cell_used[i]); }

    // build rejected points set (those not matched to global_used)
    inspection_rejected_points.clear();
    for (const auto &u : all_unique_candidates) {
        bool matched=false;
        for (const auto &g : global_used) {
            double dx = g.x - u.x; double dy = g.y - u.y; double dist = std::sqrt(dx*dx + dy*dy);
            if (dist <= GLOBAL_DUP_RADIUS) { matched = true; break; }
        }
        if (!matched) inspection_rejected_points.push_back(QPointF(u.x, u.y));
    }

    // extract dirs
    auto getCell = [&](int gx_i, int gy_i) -> double {
        if (gx_i < 0 || gx_i >= gx || gy_i < 0 || gy_i >= gy) {
            return 0.0;
        }
        return cell_hfd[gy_i * gx + gx_i];
    };
    std::vector<double> dirs(8, 0.0);
    dirs[0] = getCell(2,0);
    dirs[1] = getCell(4,0);
    dirs[2] = getCell(4,2);
    dirs[3] = getCell(4,4);
    dirs[4] = getCell(2,4);
    dirs[5] = getCell(0,4);
    dirs[6] = getCell(0,2);
    dirs[7] = getCell(0,0);
    double center_hfd = getCell(2,2);

    // map counts
    std::vector<int> detected_dirs(8,0), used_dirs(8,0), rejected_dirs(8,0);
    auto getCount = [&](const std::vector<int> &vec, int gx_i, int gy_i) -> int {
        if (gx_i < 0 || gx_i >= gx || gy_i < 0 || gy_i >= gy) {
            return 0;
        }
        return vec[gy_i * gx + gx_i];
    };
    detected_dirs[0] = getCount(cell_detected, 2,0);
    detected_dirs[1] = getCount(cell_detected, 4,0);
    detected_dirs[2] = getCount(cell_detected, 4,2);
    detected_dirs[3] = getCount(cell_detected, 4,4);
    detected_dirs[4] = getCount(cell_detected, 2,4);
    detected_dirs[5] = getCount(cell_detected, 0,4);
    detected_dirs[6] = getCount(cell_detected, 0,2);
    detected_dirs[7] = getCount(cell_detected, 0,0);

    used_dirs[0] = getCount(cell_used, 2,0);
    used_dirs[1] = getCount(cell_used, 4,0);
    used_dirs[2] = getCount(cell_used, 4,2);
    used_dirs[3] = getCount(cell_used, 4,4);
    used_dirs[4] = getCount(cell_used, 2,4);
    used_dirs[5] = getCount(cell_used, 0,4);
    used_dirs[6] = getCount(cell_used, 0,2);
    used_dirs[7] = getCount(cell_used, 0,0);

    rejected_dirs[0] = getCount(cell_rejected, 2,0);
    rejected_dirs[1] = getCount(cell_rejected, 4,0);
    rejected_dirs[2] = getCount(cell_rejected, 4,2);
    rejected_dirs[3] = getCount(cell_rejected, 4,4);
    rejected_dirs[4] = getCount(cell_rejected, 2,4);
    rejected_dirs[5] = getCount(cell_rejected, 0,4);
    rejected_dirs[6] = getCount(cell_rejected, 0,2);
    rejected_dirs[7] = getCount(cell_rejected, 0,0);

    int center_detected = getCount(cell_detected, 2,2);
    int center_used = getCount(cell_used, 2,2);
    int center_rejected = getCount(cell_rejected, 2,2);

    // fill result
    res.dirs = std::move(dirs);
    res.center_hfd = center_hfd;
    res.detected_dirs = std::move(detected_dirs);
    res.used_dirs = std::move(used_dirs);
    res.rejected_dirs = std::move(rejected_dirs);
    res.center_detected = center_detected;
    res.center_used = center_used;
    res.center_rejected = center_rejected;
    res.used_points = std::move(inspection_used_points);
    res.used_radii = std::move(inspection_used_radii);
    res.rejected_points = std::move(inspection_rejected_points);
    res.cell_eccentricity = std::move(per_cell_ecc);
    res.cell_major_angle = std::move(per_cell_angle);

    return res;
}
