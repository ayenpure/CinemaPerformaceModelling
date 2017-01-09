#include <iostream>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <sstream>
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
#include "CameraPositions.h"
#include "Utilities.h"
#include "TriangleOperations.h"
#include "MatrixOperations.h"
#include "Camera.h"
#include "Screen.h"
#include "RenderFunctions.h"

using std::cerr;
using std::endl;
using std::min;
using std::max;
using std::abs;
using std::pow;
using std::tan;
using std::sin;

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

int main(int argc, char *argv[]) {
	int no_of_procs = 56, num_cameras = 0;
	std::vector<Triangle> triangles;
	if(argc < 2) {
		cout << "Incorrect number of arguments for execution" << endl;
		exit (EXIT_FAILURE);
	} else if (argc == 2) {
		triangles = GetTrianglesFromFiles(no_of_procs);
	}  else if (argc == 3) {
		no_of_procs = 64;
		triangles = GetTriangles();
	}
	int config_id = atoi(argv[1]);
	double** camera_positions = get_camera_positions(config_id,&num_cameras);
	int active_pixels[num_cameras];
	int no_of_triangles = triangles.size();
	for (int i = 0; i < num_cameras; i++)
		active_pixels[i] = 0;
	for (int cam_index = 0; cam_index < num_cameras; cam_index++) {
		vtkImageData *image = NewImage(1000, 1000);
		unsigned char *buffer = (unsigned char *) image->GetScalarPointer(0, 0,
				0);
		int npixels = 1000 * 1000;
		for (int i = 0; i < npixels * 3; i++)
			buffer[i] = 0;
		double *depth_buffer = (double*)malloc(npixels*sizeof(double));
		for (int i = 0; i < npixels; i++)
			depth_buffer[i] = -1;
		Screen screen;
		screen.buffer = buffer;
		screen.depth_buffer = depth_buffer;
		screen.width = 1000;
		screen.height = 1000;
		double camera_position[3], focus_point[3];
		get_camera_position_and_focus(config_id, cam_index, num_cameras,
			 camera_positions, camera_position, focus_point);
		Camera camera = GetCamera(camera_position, focus_point);
		Matrix camera_transform = camera.CameraTransform();
		//cout<<"\nCamera Transform Matrix :\n";camera_transform.Print(std::cout);
		Matrix view_transform = camera.ViewTransform();
		//cout<<"\nView Transform Matrix :\n";view_transform.Print(std::cout);
		Matrix device_transform = camera.DeviceTransform(screen);
		//cout<<"\nDevice Transform Matrix :\n";device_transform.Print(std::cout);
		Matrix composite = get_total_transform_matrix(camera_transform,
				view_transform, device_transform);
		//cout<<"\nComposite Matrix :\n";composite.Print(std::cout);
		for (int vecIndex = 0; vecIndex < triangles.size(); vecIndex++) {
			//cout << "Triangle #" << vecIndex << endl;
			Triangle t = triangles[vecIndex];
			//print_triangle(t);
			transformTriangle(&t, composite, camera);
			//print_triangle(t);
			if (t.is_flat_bottom_triangle()) {
				/*triangle_pixels[vecIndex] = triangle_pixels[vecIndex] + */scan_line(&t, &screen);
			} else {
				Triangle t1, t2;
				t.split_triangle(&t1, &t2);
				/*triangle_pixels[vecIndex] = triangle_pixels[vecIndex] + */scan_line(&t1, &screen);
				/*triangle_pixels[vecIndex] = triangle_pixels[vecIndex] + */scan_line(&t2, &screen);
			}
		}
		std::ostringstream oss;
		oss << "camera_" << cam_index;
		active_pixels[cam_index] = 0;
		for(int pxl =0; pxl < 3*npixels; pxl+=3) {
			if(!(buffer[pxl] == 0 &&
				buffer[pxl+1] == 0 &&
				buffer[pxl+2] == 0 ))
				active_pixels[cam_index]++;
		}
		active_pixels[cam_index] = active_pixels[cam_index] / no_of_procs;
		WriteImage(image, oss.str().c_str());
		oss.str("");
		oss.clear();
	}
	for (int i = 0; i < num_cameras; i++) {
		cout << active_pixels[i] << endl;
	}
}