#ifndef KEY_VIEWS_CALCULATOR_H
#define KEY_VIEWS_CALCULATOR_H

#include <unordered_set>

class KeyViewsCalculator {

private:
	Intrinsics intrinsics;
	Extrinsics extrinsics;
	bool verbose = verbose;

	std::vector<int> ids;
	int maxNrKeyViews;
	// create new intrinsics, for a small resolution camera
	int width;
	int height;
	int N;    // width * height
	float fx;
	float fy;
	float cx;
	float cy;

	std::map<int, std::map<int, std::vector<bool>>> overlap;

public:
	int nrMvsNeighbors = 4;
	std::vector<int> keyCameras;
	std::map<int, std::vector<int>> mvsNeighbors; // per camera, calculate which other cameras would be good stereo pairs

	KeyViewsCalculator(Intrinsics intrinsics, Extrinsics extrinsics, bool verbose) :
		intrinsics(intrinsics),
		extrinsics(extrinsics),
		verbose(verbose) {
		maxNrKeyViews = 100; // TODO

		// create new intrinsics, for a small resolution camera
		float aspect = static_cast<float>(intrinsics.height) / intrinsics.width;
		if (aspect > 1) {
			// height > width
			height = 10;
			width = height / aspect;
		}
		else {
			// width > height
			width = 10;
			height = width * aspect;
		}
		fx = intrinsics.fx / intrinsics.width * width;
		fy = intrinsics.fy / intrinsics.width * width;
		cx = width * 0.5f;
		cy = height * 0.5f;
		N = width * height;

		nrMvsNeighbors = std::min(nrMvsNeighbors, static_cast<int>(extrinsics.imageNames.size()) - 1);
		ids = extrinsics.imageIds;
	}

	void EstimateOverlapBetweenCameras() {

		// for each mainId, overlap(mainId) will contain a map where the key is a neighborId,
		// and the value is a vector of bools, indicating which of the points projected
		// from neighor to main lie within the image bounds of main (a sign of overlap).
		for (int& id : ids) {
			overlap[id] = std::map<int, std::vector<bool>>(); // initialize
		}

		// but first, populate overlap with (mainId, neighborId) pairs for which holds that
		// the angle between these cameras < 20 degrees and the distance between cameras is minimal
		for (int& mainId : ids) {

			std::vector<std::pair<int, float>> neighborIdsAndDistances;
			for (int& neighborId : ids) {
				float dot_product = std::min(std::max(-1.0f, glm::dot(extrinsics.poses[mainId].forward, extrinsics.poses[neighborId].forward)), 1.0f);
				float distance = glm::length(extrinsics.poses[mainId].pos - extrinsics.poses[neighborId].pos);

				// if viewing angle beteen mainId and neighborId < 20 degrees, it is seen as a good neighbor
				if (dot_product > 0.939f) {
					neighborIdsAndDistances.push_back(std::pair<int, float>(neighborId, distance));
				}
			}

			// sort from smallest distance to largest
			std::sort(neighborIdsAndDistances.begin(), neighborIdsAndDistances.end(), [](const auto& a, const auto& b) {
				return a.second < b.second;
				});

			// populate overlap with up to 11 neighbors (including itself)
			for (int i = 0; i < std::min(11, (int)neighborIdsAndDistances.size()); i++) {
				int neighborId = neighborIdsAndDistances[i].first;
				overlap[mainId][neighborId] = std::vector<bool>();
			}
		}

		// pre-calculate a small set of points evenly distributed within a width x height image
		float z = 10;
		std::vector<glm::vec4> xyzw_local;
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				xyzw_local.push_back(glm::vec4(
					(col + 0.5f - cx) / fx * z,
					(row + 0.5f - cy) / fy * z,
					z,
					1
				));
			}
		}

		// pre-calculate image boundaries, because:
		// 0 < x/z * fx + cx < width is same as  -cx / fx < x/z < (width - cx) / fx
		// 0 < y/z * fy + cy < height is same as  -cy / fy < y/z < (height - cy) / fy
		glm::vec2 lowerBound(-cx / fx, -cy / fy);
		glm::vec2 upperBound((width - cx) / fx, (height - cy) / fy);

		// now calculate the bool values by, for each (mainId, neighborId) pair already in overlap,
		// creating a small point cloud (N points) for neighborId and projecting that onto mainId
		for (int& neighborId : ids) {

			// create point cloud
			std::vector<glm::vec4> xyzw_world;
			for (glm::vec4& xyzw : xyzw_local) {
				xyzw_world.push_back(extrinsics.poses[neighborId].model * xyzw);
			}
			
			// project points onto mainId, so that we can overwrite overlap[mainId][neighborId]
			for (int& mainId : ids) {
				if (mainId == neighborId) {
					// fill with trues
					overlap[mainId][neighborId] = std::vector<bool>(N, true);
					continue;
				}
				if (overlap[mainId].find(neighborId) == overlap[mainId].end()) continue; // skip if neighborId not in overlap[mainId]

				// world to local space
				std::vector<glm::vec4> xyzw_local2;
				for (glm::vec4& xyzw : xyzw_world) {
					xyzw_local2.push_back(extrinsics.poses[mainId].view * xyzw);
				}

				std::vector<bool> mask(xyzw_local2.size(), true);
				for (int i = 0; i < xyzw_local2.size(); i++) {
					glm::vec4 xyzw = xyzw_local2[i];
					// mask = false for points with depth is < 0.1
					if (xyzw.z < 0.1f) {
						mask[i] = false;
						continue;
					}
					// mask = false for points that do not lie within image bounds
					float x = xyzw.x / xyzw.z;
					float y = xyzw.y / xyzw.z;
					if (x < lowerBound.x || x > upperBound.x || y < lowerBound.y || y > upperBound.y) {
						mask[i] = false;
						continue;
					}
				}
				overlap[mainId][neighborId] = mask;
			}
		}

	}

	bool CalculateKeyCameras() {

		if (verbose) printf("Choice of key cameras:\n");

		// fill keyCameras with a minimal subset of ids, so that these key cameras have as much overlap with all the other cameras as possible.
		keyCameras.clear();
		std::map<int, std::vector<bool>> keyCamOverlap;
		int best_nr_new_overlaps = 0;
		int best_new_keyCam = ids[0];
		int totalCoverage = 0;
		int maxPossibleCoverage = ids.size() * N;

		while (true) {

			// update keyCameras
			keyCameras.push_back(best_new_keyCam);

			// merge overlap[best_new_keyCam] into keyCamOverlap
			for (auto& tup : overlap[best_new_keyCam]) {
				int neighborId = tup.first;
				std::vector<bool> keyCam_neighbor_overlap = tup.second;

				// check if neighborId already in keyCamOverlap
				if (keyCamOverlap.find(neighborId) != keyCamOverlap.end()) {
					// update keyCamOverlap[neighborId]
					for (int i = 0; i < N; i++) {
						if (!keyCamOverlap[neighborId][i] && keyCam_neighbor_overlap[i]) {
							keyCamOverlap[neighborId][i] = true;
							totalCoverage++;
						}
					}
				}
				else {
					// add neighborId to keyCamOverlap
					keyCamOverlap[neighborId] = keyCam_neighbor_overlap;
					totalCoverage += N;
				}
			}

			float totalCoverageRatio = float(totalCoverage) / maxPossibleCoverage;
			if(verbose) printf("%s, coverage: %f\n", extrinsics.imageNames[best_new_keyCam].c_str(), totalCoverageRatio);

			// break condition
			if (keyCameras.size() >= maxNrKeyViews || totalCoverageRatio >= 0.9f) break;

			// find another key camera
			best_new_keyCam = -1;
			best_nr_new_overlaps = 0;
			for (int& possibleKeyCam : ids) {
				if (std::find(keyCameras.begin(), keyCameras.end(), possibleKeyCam) != keyCameras.end()) continue;

				// calculate number of new overlaps
				int nr_new_overlaps = 0;
				for (auto& tup : overlap[possibleKeyCam]) {
					int neighborId = tup.first;

					// check if neighborId already in any keyCamOverlap
					if (keyCamOverlap.find(neighborId) != keyCamOverlap.end()) {
						for (int i = 0; i < N; i++) {
							if (!keyCamOverlap[neighborId][i] && overlap[possibleKeyCam][neighborId][i]) nr_new_overlaps++;
						}
					}
					else {
						nr_new_overlaps += N;
					}
				}

				if (nr_new_overlaps > best_nr_new_overlaps) {
					best_nr_new_overlaps = nr_new_overlaps;
					best_new_keyCam = possibleKeyCam;
				}
			}

			if (best_new_keyCam < 0) {
				printf("Error: something went wrong while trying to find the key cameras. There is no good camera to add.\n");
				return false;
			}
		}

		SortIdsByName(keyCameras);

		printf("Total nr key cameras : % d\n", keyCameras.size());
		return true;
	}

	void CalculateMvsNeighbors() {

		if (verbose) printf("Choice of MVS neighbors per key camera:\n");

		// Because we want to project the key cameras to its neighbors,
		// overlap will now be used as: overlap[neighborId][keyCamId].
		// We need to find 4 neighborIds so that overlap[neighborId][keyCamId] has as many points set to true as possible.
		for (int& keyCamId : keyCameras) {

			// find neighborIds for which overlap[neighborId][keyCamId] exists
			std::vector<int> possibleNeighborIds;
			std::vector<float> cos_angles;
			std::vector<std::vector<bool>> keyCamOverlap;
			for (int& neighborId : ids) {
				if (neighborId != keyCamId && overlap[neighborId].find(keyCamId) != overlap[neighborId].end()) {
					cos_angles.push_back(std::min(std::max(-1.0f, glm::dot(extrinsics.poses[keyCamId].forward, extrinsics.poses[neighborId].forward)), 1.0f));
					possibleNeighborIds.push_back(neighborId);
					keyCamOverlap.push_back(overlap[neighborId][keyCamId]);
				}
			}

			// solve this minimal set problem
			std::vector<int> selectedRows;
			SelectMinimalCamerasToCoverEveryPoint(keyCamOverlap, cos_angles, selectedRows);

			// try to fill selectedRows to contain nrMvsNeighbors elements
			while (selectedRows.size() < std::min(nrMvsNeighbors, (int)possibleNeighborIds.size())) {
				// add the row (not yet in selectedRows) with the most trues
				int best_row = 0;
				int best_nr_trues = 0;
				for (int row = 0; row < possibleNeighborIds.size(); row++) {
					// make sure row not yet in selectedRows
					if (std::find(selectedRows.begin(), selectedRows.end(), row) != selectedRows.end()) continue;
					
					int nr_trues = 0;
					for (int i = 0; i < N; i++) if (keyCamOverlap[row][i]) nr_trues++;
					if (nr_trues > best_nr_trues) {
						best_row = row;
						best_nr_trues = nr_trues;
					}
				}
				selectedRows.push_back(best_row);
			}

			// convert selectedRows back to neighborIds
			std::vector<int> tmp;
			for (int& row : selectedRows) tmp.push_back(possibleNeighborIds[row]);
			possibleNeighborIds = tmp;

			// fill mvsNeighbors[keyCamId]
			mvsNeighbors[keyCamId] = std::vector<int>();
			if(verbose) printf("%s: ", extrinsics.imageNames[keyCamId].c_str());
			for (int i = 0; i < std::min(nrMvsNeighbors, (int)possibleNeighborIds.size()); i++) {
				mvsNeighbors[keyCamId].push_back(possibleNeighborIds[i]);
				if (verbose) printf("%s, ", extrinsics.imageNames[possibleNeighborIds[i]].c_str());
			}
			if (verbose) printf("\n");
			if (verbose && mvsNeighbors[keyCamId].size() != 4) {
				printf("Warning: key camera %d only has %d neighbors.\n", keyCamId, mvsNeighbors[keyCamId].size());
			}
		}
	}

	void Cleanup() {
		overlap.clear();
	}

private:

	void SortIdsByName(std::vector<int>& ids) {
		std::vector<std::pair<int, std::string>> id_name;
		for (int& id : ids) {
			id_name.push_back(std::pair<int, std::string>(id, extrinsics.imageNames[id]));
		}
		// sort by name
		std::sort(id_name.begin(), id_name.end(), [](const auto& a, const auto& b) {
			return a.second < b.second;
			});
		ids.clear();
		for (auto& pair : id_name) {
			ids.push_back(pair.first);
		}
	}

	static void SelectMinimalCamerasToCoverEveryPoint(const std::vector<std::vector<bool>>& matrix, std::vector<float> heuristic, /*out*/ std::vector<int>& selectedRows) {
		// Select all rows so that each column has "true" at least twice.
		// If there is multiple options, choose the row with the highest heuristic[row].

		int N = matrix.size(), M = matrix[0].size();
		std::unordered_set<int> selectedRowSet;
		std::vector<int> columnCoverage(M, 0); // Track how many times each column is covered

		// Track which rows contribute to which columns
		std::vector<std::unordered_set<int>> rowCoverage(N);
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < M; ++j) {
				if (matrix[i][j]) rowCoverage[i].insert(j);
			}
		}

		while (true) {
			// Find the row that contributes the most towards covering columns at least twice
			int bestRow = -1, maxNewCoverage = 0;
			float maxHeuristic = -1;
			for (int row = 0; row < N; ++row) {
				if (selectedRowSet.count(row)) continue; // Skip already selected rows

				int newCoverage = 0;
				for (int col : rowCoverage[row]) {
					if (columnCoverage[col] < 2) newCoverage++; // Count only columns that aren't yet covered twice
				}
				if (newCoverage > maxNewCoverage) {
					maxNewCoverage = newCoverage;
					maxHeuristic = heuristic[row];
					bestRow = row;
				}
				else if (newCoverage == maxNewCoverage && heuristic[row] > maxHeuristic) {
					maxHeuristic = heuristic[row];
					bestRow = row;
				}
			}

			// If no row adds coverage, stop
			if (bestRow == -1) break;

			// Add the selected row
			selectedRows.push_back(bestRow);
			selectedRowSet.insert(bestRow);
			for (int col : rowCoverage[bestRow]) {
				columnCoverage[col]++;
			}

			// Check if all columns are covered at least twice
			bool allCoveredTwice = std::all_of(columnCoverage.begin(), columnCoverage.end(), [](int c) { return c >= 2; });
			if (allCoveredTwice) break;
		}
	}

};



#endif // !KEY_VIEWS_CALCULATOR_H
