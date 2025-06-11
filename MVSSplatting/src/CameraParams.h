#ifndef CAMERA_PARAMS_H
#define CAMERA_PARAMS_H

#include <sstream>

class Intrinsics {
public:
	std::string camerasBinPath;
	int width = 0;
	int height = 0;
	float fx = 0;
	float fy = 0;
	float cx = 0;
	float cy = 0;

	Intrinsics() {}

	Intrinsics(int width, int height, float fx, float fy, float cx, float cy) : 
		width(width), height(height), fx(fx), fy(fy), cx(cx), cy(cy) {}

	Intrinsics(std::string camerasBinPath) {
		this->camerasBinPath = camerasBinPath;
	}

	bool Init() {
		std::ifstream file(camerasBinPath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Error: Could not open " << camerasBinPath << std::endl;
			return false;
		}

		size_t nrCameras;
		file.read(reinterpret_cast<char*>(&nrCameras), sizeof(nrCameras));

		while (file.peek() != EOF) {
			uint32_t camera_id;
			uint32_t model_id;
			uint64_t width_, height_;
			std::vector<double> params;

			file.read(reinterpret_cast<char*>(&camera_id), sizeof(camera_id));
			file.read(reinterpret_cast<char*>(&model_id), sizeof(model_id));
			file.read(reinterpret_cast<char*>(&width_), sizeof(width_));
			file.read(reinterpret_cast<char*>(&height_), sizeof(height_));

			width = width_;
			height = height_;
			
			// Only allow models with no distortion
			if (model_id == 0) { // SIMPLE_PINHOLE
				params.resize(3);
				file.read(reinterpret_cast<char*>(params.data()), sizeof(double) * 3);
				fx = params[0];
				cx = params[1];
				cy = params[2];
			}
			else if (model_id == 1) { // PINHOLE 
				params.resize(4);
				file.read(reinterpret_cast<char*>(params.data()), sizeof(double) * 4);
				fx = params[0];
				fy = params[1];
				cx = params[2];
				cy = params[3];
			}
			else {
				std::cerr << "Unsupported camera model ID: " << model_id << "\nExpected 0 (SIMPLE_PINHOLE) or 1 (PINHOLE)\n";
				return false;
			}
			if (fy == 0) fy = fx;
			printf("res = [%d, %d], f = [%.2f, %.2f], c = [%.2f, %.2f]\n", width, height, fx, fx, cx, cy);
		}

		return true;
	}
};

struct CameraPose {
	glm::mat4 R;
	glm::vec3 T;
	glm::mat4 model;
	glm::mat4 view;
	glm::vec3 pos;
	glm::vec3 forward;
};

class Extrinsics {
public:
	bool eval;
	std::string imagesBinPath;
	std::vector<int> imageIds;
	std::unordered_map<int, std::string> imageNames;
	std::unordered_map<int, CameraPose> poses; // needed for 'view' matrix

	Extrinsics(std::string imagesBinPath, bool eval) : imagesBinPath(imagesBinPath), eval(eval){}

	bool Init() {

		std::ifstream file(imagesBinPath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Error: Could not open " << imagesBinPath << std::endl;
			return false;
		}

		size_t num_reg_images;
		file.read(reinterpret_cast<char*>(&num_reg_images), sizeof(num_reg_images));

		while (file.peek() != EOF) {
			uint32_t image_id;
			double q[4];             // qw, qx, qy, qz
			double t[3];             // tx, ty, tz
			uint32_t camera_id;
			std::string image_name;

			file.read(reinterpret_cast<char*>(&image_id), sizeof(uint32_t));
			file.read(reinterpret_cast<char*>(&q), sizeof(double) * 4);
			file.read(reinterpret_cast<char*>(&t), sizeof(double) * 3);
			file.read(reinterpret_cast<char*>(&camera_id), sizeof(uint32_t));

			// Read null-terminated image name
			char c;
			while (file.get(c) && c != '\0') {
				image_name += c;
			}

			// don't need points2D so seek forward 24 bytes per point
			uint64_t num_points2D;
			file.read(reinterpret_cast<char*>(&num_points2D), sizeof(uint64_t));
			file.seekg(static_cast<std::streamoff>(num_points2D) * 24, std::ios_base::cur);

			glm::mat4 R = glm::mat4_cast(glm::quat(q[0], q[1], q[2], q[3]));
			glm::vec3 T = glm::vec3(t[0], t[1], t[2]);

			glm::mat4 view = R;
			view[3] = glm::vec4(T, 1);
			glm::mat4 model = glm::inverse(view);

			glm::vec3 pos = glm::vec3(model[3]);
			glm::vec3 forward = -glm::normalize(glm::vec3(model[2]));

			// save in unordered_map
			imageIds.push_back(image_id);
			poses[image_id] = { R, T, model, view, pos, forward };
			imageNames[image_id] = image_name;
		}

		// sort images alphabetically
		SortIdsByName();

		if (eval) {
			printf("Using train/test split, so starting from the 3rd image, ignoring every 8th image\n");
			// delete every 8th id, name and pose
			std::vector<int> newImageIds;
			for (int i = 0; i < imageIds.size(); i++) {
				if (i % 8 != 2) {
					newImageIds.push_back(imageIds[i]);
				}
				else{
					imageNames.erase(imageIds[i]);
					poses.erase(imageIds[i]);
				}
			}
			imageIds = newImageIds;
		}

		return true;
	}

private:
	void SortIdsByName() {
		std::vector<std::pair<int, std::string>> id_name;
		for (int& id : imageIds) {
			id_name.push_back(std::pair<int, std::string>(id, imageNames[id]));
		}
		// sort by name
		std::sort(id_name.begin(), id_name.end(), [](const auto& a, const auto& b) {
			return a.second < b.second;
		});
		imageIds.clear();
		for (auto& pair : id_name) {
			imageIds.push_back(pair.first);
		}
	}
};

class SfmPoints {
public:
	std::string points3DBinPath;
	std::unordered_map<int, std::vector<glm::vec3>> points;
	std::unordered_map<int, std::vector<glm::vec3>> colors;
	std::unordered_map<int, glm::vec2> depthRanges; // [near, far] per image id

	SfmPoints() {};
	SfmPoints(std::string points3DBinPath) : points3DBinPath(points3DBinPath) { }

	bool Init(Extrinsics extrinsics) {

		// setup depth range calculation
		std::unordered_map<int, glm::vec4> views;
		for (int& id : extrinsics.imageIds) {
			glm::mat4 view = extrinsics.poses[id].view;
			views[id] = glm::vec4(view[0][2], view[1][2], view[2][2], view[3][2]);
			depthRanges[id] = glm::vec2(9999, -1);
		}

		std::ifstream file(points3DBinPath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Error: Could not open " << points3DBinPath << std::endl;
			return false;
		}

		size_t nrPoints;
		file.read(reinterpret_cast<char*>(&nrPoints), sizeof(nrPoints));
		printf("nrPoints in points3D.bin = %d\n", (int)nrPoints);

		while (file.peek() != EOF) {
			uint64_t point3D_id;
			double x, y, z;
			uint8_t r, g, b;
			double error;
			uint64_t track_length;

			file.read(reinterpret_cast<char*>(&point3D_id), sizeof(point3D_id));
			file.read(reinterpret_cast<char*>(&x), sizeof(x));
			file.read(reinterpret_cast<char*>(&y), sizeof(y));
			file.read(reinterpret_cast<char*>(&z), sizeof(z));
			file.read(reinterpret_cast<char*>(&r), sizeof(r));
			file.read(reinterpret_cast<char*>(&g), sizeof(g));
			file.read(reinterpret_cast<char*>(&b), sizeof(b));
			file.read(reinterpret_cast<char*>(&error), sizeof(error));
			file.read(reinterpret_cast<char*>(&track_length), sizeof(track_length));

			glm::vec3 pos = glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
			glm::vec3 col = glm::vec3(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)) / 255.0f;

			bool found_valid_id = false;
			for (int i = 0; i < track_length; ++i) {
				uint32_t image_id, point2D_idx;
				file.read(reinterpret_cast<char*>(&image_id), sizeof(image_id));
				file.read(reinterpret_cast<char*>(&point2D_idx), sizeof(point2D_idx));

				if (std::find(extrinsics.imageIds.begin(), extrinsics.imageIds.end(), image_id) != extrinsics.imageIds.end()) {
					if (!found_valid_id) {
						found_valid_id = true;
						points[image_id].emplace_back(pos);
						colors[image_id].emplace_back(col);
					}
					float z = glm::dot(glm::vec3(views[image_id]), pos) + views[image_id].w;
					depthRanges[image_id].x = std::min(depthRanges[image_id].x, z); // near
					depthRanges[image_id].y = std::max(depthRanges[image_id].y, z); // far
				}
			}
		}

		return true;
	}
};

#endif // !CAMERA_PARAMS_H
