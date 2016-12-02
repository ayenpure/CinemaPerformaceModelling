#include <iostream>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkFloatArray.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkCellArray.h>
#include <vtkDataSetWriter.h>
#include <cmath>

using std::cerr;
using std::endl;
using std::abs;

double ceil441(double f) {
	return ceil(f - 0.00001);
}

double floor441(double f) {
	return floor(f + 0.00001);
}

double interpolate(double point_1, double point_2, double value_1,
		double value_2, double quest_point) {
	double diff = point_2 - point_1;
	double change_prop =
			(diff != 0) ? (quest_point - point_1) / (point_2 - point_1) : 0;
	double value_at_quest = value_1 + (change_prop * (value_2 - value_1));
	return value_at_quest;
}

void normalize_vector(double* quest_normal) {
	double norm = sqrt(
			(quest_normal[0] * quest_normal[0])
					+ (quest_normal[1] * quest_normal[1])
					+ (quest_normal[2] * quest_normal[2]));
	quest_normal[0] = quest_normal[0] / norm;
	quest_normal[1] = quest_normal[1] / norm;
	quest_normal[2] = quest_normal[2] / norm;
}

double dot_product(double* vector_1, double* vector_2) {
	return (vector_1[0] * vector_2[0]) + (vector_1[1] * vector_2[1])
			+ (vector_1[2] * vector_2[2]);
}

void cross_product(double* vector_1, double* vector_2, double *cross_vec) {
	double product[3] = { (vector_1[1] * vector_2[2])
			- (vector_1[2] * vector_2[1]), (vector_1[2] * vector_2[0])
			- (vector_1[0] * vector_2[2]), (vector_1[0] * vector_2[1])
			- (vector_1[1] * vector_2[0]) };
	memcpy(cross_vec, product, 3 * sizeof(double));
}

class Screen {
public:
	unsigned char *buffer;
	int width, height;

	void find_pixel_and_color(int x, int y, double *color) {
		/*
		 * Ensure the pixels to be painted are in the frame.
		 */
		if ((x >= 0 && x < width) && (y >= 0 && y < height)) {
			int buffer_index = (y * 3 * width) + (x * 3);
			if (buffer_index < width * height * 3) {
				/*double shading_amount = calculate_phong_shading(lp,
						view_direction, current_normal);*/
				buffer[buffer_index++] = color[0];
				buffer[buffer_index++] = color[1];
				buffer[buffer_index] = color[2];
			}
		}
	}

	/*
	 * This method calculates the color for a certain pixel in a trangle given
	 * its intercepts for a scanline and the colors at the intercepts.
	 */
	void calculate_color_for_pixel(double left_intercept,
			double right_intercept, double current_x_pos,
			double *color_at_left_intercept, double *color_at_right_intercept,
			double *current_color) {

		current_color[0] = interpolate(left_intercept, right_intercept,
				color_at_left_intercept[0], color_at_right_intercept[0],
				current_x_pos);
		current_color[1] = interpolate(left_intercept, right_intercept,
				color_at_left_intercept[1], color_at_right_intercept[1],
				current_x_pos);
		current_color[2] = interpolate(left_intercept, right_intercept,
				color_at_left_intercept[2], color_at_right_intercept[2],
				current_x_pos);
	}
};

vtkImageData *
NewImage(int width, int height) {
	vtkImageData *img = vtkImageData::New();
	img->SetDimensions(width, height, 1);
	img->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
	return img;
}

void WriteImage(vtkImageData *img, const char *filename) {
	std::string full_filename = filename;
	full_filename += ".png";
	vtkPNGWriter *writer = vtkPNGWriter::New();
	writer->SetInputData(img);
	writer->SetFileName(full_filename.c_str());
	writer->Write();
	writer->Delete();
}

class Triangle {
public:
	double X[3];
	double Y[3];
	double Z[3];
	double colors[3][3];
	double normals[3][3];


	double getlowestY() {
		return ((Y[0] < Y[1]) ? Y[0] : ((Y[1] < Y[2]) ? Y[1] : Y[2]));
	}

	double gethighestY() {
		return ((Y[0] > Y[1]) ? Y[0] : ((Y[1] > Y[2]) ? Y[1] : Y[2]));
	}

	double getlowestX() {
		return ((X[0] < X[1]) ? X[0] : ((X[1] < X[2]) ? X[1] : X[2]));
	}

	double gethighestX() {
		return ((X[0] > X[1]) ? X[0] : ((X[1] > X[2]) ? X[1] : X[2]));
	}

	double getlowestZ() {
		return ((Z[0] < Z[1]) ? Z[0] : ((Z[1] < Z[2]) ? Z[1] : Z[2]));
	}

	double gethighestZ() {
		return ((Z[0] > Z[1]) ? Z[0] : ((Z[1] > Z[2]) ? Z[1] : Z[2]));
	}

	void calculate_normals() {
		for (int i = 0; i < 3; i++) {
			int adj_1 = (i + 1) % 3;
			int adj_2 = (i + 2) % 3;
			double normal[3];
			if((X[adj_1] == X[adj_2] && X[adj_1] == X[i] && X[adj_2] == X[i])
			&& (Y[adj_1] == Y[adj_2] && Y[adj_1] == Y[i] && Y[adj_2] == Y[i])
			&& (Z[adj_1] == Z[adj_2] && Z[adj_1] == Z[i] && Z[adj_2] == Z[i])){
				normal[0] = X[i];
				normal[1] = Y[i];
				normal[2] = Z[i];
			} else if (X[adj_1] == X[adj_2] && Y[adj_1] == Y[adj_2] && Z[adj_1] == Z[adj_2] ) {
				normal[0] = (X[adj_1] + X[i]) /2;
				normal[1] = (Y[adj_1] + Y[i]) /2;
				normal[2] = (Z[adj_1] + Z[i]) /2;
			} else if (X[adj_1] == X[i] && Y[adj_1] == Y[i] && Z[adj_1] == Z[i] ) {
				normal[0] = (X[adj_2] + X[i]) /2;
				normal[1] = (Y[adj_2] + Y[i]) /2;
				normal[2] = (Z[adj_2] + Z[i]) /2;
			} else if (X[i] == X[adj_2] && Y[i] == Y[adj_2] && Z[i] == Z[adj_2] ) {
				normal[0] = (X[adj_1] + X[adj_2]) /2;
				normal[1] = (Y[adj_1] + Y[adj_2]) /2;
				normal[2] = (Z[adj_1] + Z[adj_2]) /2;
			} else {
				double adj_1_vector[3] = { X[adj_1] - X[i], Y[adj_1] - Y[i],
					Z[adj_1] - Z[i] };
				normalize_vector(adj_1_vector);
				double adj_2_vector[3] = { X[adj_2] - X[i], Y[adj_2] - Y[i],
					Z[adj_2] - Z[i] };
				normalize_vector(adj_2_vector);
				cross_product(adj_1_vector, adj_2_vector, normal);
			}
			normalize_vector(normal);
			normals[i][0] = normal[0];
			normals[i][1] = normal[1];
			normals[i][2] = normal[2];
			cout << normal[0] << ", " << normal[1] << ", " << normal[2] << endl;
		}
		cout << endl;
	}
};

std::vector<Triangle> GetTriangles(const char *filename) {

	vtkPolyDataReader *rdr = vtkPolyDataReader::New();
	rdr->SetFileName(filename);
	//cerr << "Reading" << endl;
	rdr->Update();
	//cerr << "Done reading" << endl;
	if (rdr->GetOutput()->GetNumberOfCells() == 0) {
		cerr << "Unable to open file!!" << endl;
		exit (EXIT_FAILURE);
	}
	vtkPolyData *pd = rdr->GetOutput();
	int numTris = pd->GetNumberOfCells();
	std::vector<Triangle> tris(numTris);
	vtkPoints *pts = pd->GetPoints();
	vtkCellArray *cells = pd->GetPolys();
	vtkFloatArray *var = (vtkFloatArray *) pd->GetPointData()->GetArray("hardyglobal");
	float *color_ptr = var->GetPointer(0);
	vtkFloatArray *n = (vtkFloatArray *) pd->GetPointData()->GetNormals();
	vtkIdType npts;
	vtkIdType *ptIds;
	int idx;
	for (idx = 0, cells->InitTraversal();
			cells->GetNextCell(npts, ptIds); idx++) {
		if (npts != 3) {
			cerr << "Non-triangles!! ???" << endl;
			exit (EXIT_FAILURE);
		}

		double *pt = NULL;
		pt = pts->GetPoint(ptIds[0]);
		tris[idx].X[0] = pt[0];
		tris[idx].Y[0] = pt[1];
		tris[idx].Z[0] = pt[2];

		pt = pts->GetPoint(ptIds[1]);
		tris[idx].X[1] = pt[0];
		tris[idx].Y[1] = pt[1];
		tris[idx].Z[1] = pt[2];

		pt = pts->GetPoint(ptIds[2]);
		tris[idx].X[2] = pt[0];
		tris[idx].Y[2] = pt[1];
		tris[idx].Z[2] = pt[2];

		double mins[4] = { 1, 2.25, 3.5, 4.75};
		double maxs[4] = { 2.25, 3.5, 4.75, 6};
		unsigned char RGB[5][3] = {
			{0, 0, 255},
			{0, 204, 255},
			{0, 153, 0},
			{255, 204, 0},
			{255, 0, 0},
		 };
		for (int j = 0; j < 3; j++) {
			float val = color_ptr[ptIds[j]];
			int r;
			 for (r = 0 ; r < 4 ; r++)
			 {
			 if (mins[r] <= val && val < maxs[r])
			 break;
			 }
			 if (r == 4)
			 {
			 cerr << "Could not interpolate color for " << val << endl;
			 exit (EXIT_FAILURE);
			 }
			double proportion = (val-mins[r]) / (maxs[r]-mins[r]);
			tris[idx].colors[j][0] = (RGB[r][0]+proportion*(RGB[r+1][0]-RGB[r][0]))/255.0;
			tris[idx].colors[j][1] = (RGB[r][1]+proportion*(RGB[r+1][1]-RGB[r][1]))/255.0;
			tris[idx].colors[j][2] = (RGB[r][2]+proportion*(RGB[r+1][2]-RGB[r][2]))/255.0;
		}
		tris[idx].calculate_normals();
	}
	return tris;
}

bool is_point_inside_triangle(double *point_of_intrsection, Triangle triangle) {

	if(triangle.X[0] == triangle.X[1] && triangle.X[1] == triangle.X[2] && triangle.X[2] == triangle.X[0]
	&& triangle.Y[0] == triangle.Y[1] && triangle.Y[1] == triangle.Y[2] && triangle.Y[2] == triangle.Y[0]
	&& triangle.Z[0] == triangle.Z[1] && triangle.Z[1] == triangle.Z[2] && triangle.Z[2] == triangle.Z[0]) {
		if(point_of_intrsection[0] != triangle.X[0]
		|| point_of_intrsection[1] != triangle.Y[0]
		|| point_of_intrsection[2] != triangle.Z[0])
			return false;
	}

	double x_max = triangle.gethighestX(),
	x_min = triangle.getlowestX(),
	y_max = triangle.gethighestY(),
	y_min = triangle.getlowestY(),
	z_max = triangle.gethighestZ(),
	z_min = triangle.getlowestZ();

	if((x_max < point_of_intrsection[0] || x_min > point_of_intrsection[0])
	|| (y_max < point_of_intrsection[1] || y_min > point_of_intrsection[1])
	|| (z_max < point_of_intrsection[2] || z_min > point_of_intrsection[2]))
		return false;

	double temp[3] = {0,0,0};
	for(int i = 0; i < 3; i++) {
		int adj_1 = (i + 1) % 3;
		double vector_1[3] = {
			triangle.X[adj_1] -  triangle.X[i],
			triangle.Y[adj_1] -  triangle.Y[i],
			triangle.Z[adj_1] -  triangle.Z[i]
		};
		double vector_2[3] = {
			point_of_intrsection[0] - triangle.X[i],
			point_of_intrsection[1] - triangle.Y[i],
			point_of_intrsection[2] - triangle.Z[i]
		};
		cross_product(vector_1, vector_2, temp);
		double dot = dot_product(triangle.normals[0], temp);
		if(dot < 0)
			return false;
	}
	return true;
}

void get_color_for_pixel(double *ray, std::vector<Triangle> triangles, double * color) {
	double camera_position[3] = {0,0,-20};
	//for(int i = 820; i < 821; i++) {
	for(int i = 0; i < triangles.size(); i++) {
		if(dot_product(ray, triangles[i].normals[0]) == 0)
			cout << "Trianlge not visible" << endl;
		else {
			double triangle_vertex[3] = {
				triangles[i].X[0],
				triangles[i].Y[0],
				triangles[i].Z[0]
			};
			double distance_form_origin = dot_product(triangles[i].normals[0],triangle_vertex);
			//float t = (dot(N, orig) + D) / dot(N, dir);
			double distance_form_camera = - ((dot_product(triangles[i].normals[0], camera_position) + distance_form_origin) / dot_product(triangles[i].normals[0], ray));
			//Vec3f Phit = orig + t * dirtrina
			double point_of_intrsection[3] = {
				camera_position[0] + distance_form_camera*ray[0],
				camera_position[1] + distance_form_camera*ray[1],
				camera_position[2] + distance_form_camera*ray[2]
			};
			if(is_point_inside_triangle(point_of_intrsection, triangles[i])) {
				cout << "Point is inside the triangle : " << i << endl;
				color[0] = 0;
				color[1] = 69;
				color[2] = 96;
			} /*else {
				cout << "Point is outside the triangle" << endl;
			}*/
		}
	}
};

int main() {
	int height = 500,width = 500;
	std::vector<Triangle> triangles = GetTriangles("hardyglobal.0.vtk");
	cout << "Number of triangles " << triangles.size() << endl;
	vtkImageData *image = NewImage(height, width);
	unsigned char *buffer = (unsigned char *) image->GetScalarPointer(0, 0, 0);
	int npixels = height * width;
	for (int i = 0; i < npixels * 3; i++)
		buffer[i] = 0;
	Screen screen;
	screen.buffer = buffer;
	screen.width = height;
	screen.height = width;

	double camera_position[3] = {0,0,-20};
	//cout << "Height :" << height << " Width :" << width;
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			double translated_x = (20*(double)x)/(double)width - 10;
			double translated_y = (20*(double)y)/(double)height - 10;
			double look_at[3] = {translated_x, translated_y, -10};
			double ray[3] = {
				look_at[0] - camera_position[0],
				look_at[1] - camera_position[1],
				look_at[2] - camera_position[2]
			};
			normalize_vector(ray);
			double color[3] = {0,0,0};
			get_color_for_pixel(ray, triangles, color);
			screen.find_pixel_and_color(x,y, color);
			if(color[0] != 0 || color[1] != 0 || color[2] != 0)
				cout << "coloring pixel " << x << ", " << y << endl;
		}
	}
	WriteImage(image, "raytracer");
	free(buffer);
}
