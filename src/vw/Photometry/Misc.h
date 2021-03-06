// __BEGIN_LICENSE__
// Copyright (C) 2006-2011 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


/// \file Misc.h

#ifndef __VW_PHOTOMETRY_MISC_H__
#define __VW_PHOTOMETRY_MISC_H__

#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <vector>

#include <vw/Image/ImageViewBase.h>
#include <vw/Cartography/GeoReference.h>
#include <vw/Photometry/Reconstruct.h>

namespace vw {
  namespace photometry {

    void upsample_uint8_image(std::string output_file, std::string input_file, int upsampleFactor);
    
  //upsamples a DEM by an upsampleFactor
  void upsample_image(std::string output_file, std::string input_file, int upsampleFactor);

  //subsamples a geo referenced tiff image by two
  void subsample_image(std::string output_file, std::string input_file);

  // Given two images and two georeferences, this function picks a set
  // of matching pixel samples between the two images.  It rejects
  // pixels that are not valid, and it should probably also reject
  // pixels that are near saturation (though it does not yet!).
  template <class ViewT>
  std::vector<Vector4> sample_images(ImageViewBase<ViewT> const& image1,
                                     ImageViewBase<ViewT> const& image2,
                                     cartography::GeoReference const& geo1,
                                     cartography::GeoReference const& geo2,
                                     int num_samples,
                                     std::string const& DEM_file,
                                     std::vector<Vector3> *normalArray,
                                     std::vector<Vector3> *xyzArray );


  /// Erases a file suffix if one exists and returns the base string
  static std::string prefix_from_filename(std::string const& filename) {
    std::string result = filename;
    int index = result.rfind(".");
    if (index != -1)
      result.erase(index, result.size());
    return result;
  }

  /// Erases a file suffix if one exists and returns the base string less3 characters
  static std::string prefix_less3_from_filename(std::string const& filename) {
    std::string result = filename;
    int index = result.rfind(".");
    if (index != -1)
      result.erase(index-3, result.size()+3);
    return result;
  }

  /// Erases a file suffix if one exists and returns the base string less3 characters
  static std::string sufix_from_filename(std::string const& filename) {
    std::string result = filename;
    int index = result.rfind("/");
    if (index != -1)
      result.erase(0, index);
    return result;
  }

  //reads the tiff DEM into a 3D coordinate
  //pos is a Vector2 of pixel coordinates, GR is georeference
  template <class ViewT>
  Vector3 pixel_to_cart (Vector2 pos, ImageViewBase<ViewT> const& img,
                         cartography::GeoReference GR);


  std::vector<std::string> parse_command_arguments(int argc, char *argv[] );

  void getTileCornersWithoutPadding(// Inputs
                                    int numCols, int numRows,
                                    cartography::GeoReference const& geoRef,
                                    double tileSize, int pixelPadding,
                                    // Outputs
                                    double & min_x, double & max_x,
                                    double & min_y, double & max_y
                                    );
  
    void applyPaddingToTileCorners(// Inputs
                                   cartography::GeoReference const& geoRef,
                                   int pixelPadding,
                                   double min_x, double max_x,
                                   double min_y, double max_y,
                                   // Outputs
                                   double & min_x_padded, double & max_x_padded,
                                   double & min_y_padded, double & max_y_padded);
    
    void readDEMTilesIntersectingBox(// Inputs
                                     double noDEMDataValue,
                                     Vector2 boxNW, Vector2 boxSE,
                                     std::vector<std::string> const& DEMTiles,
                                     // Outputs
                                     ImageView<PixelGray<float> > & combinedDEM,
                                     cartography::GeoReference    & combinedDEM_geo);

    void listTifsInDir(const std::string & dirName,
                       std::vector<std::string> & tifsInDir
                       );

    void writeSunAndSpacecraftPosition(std::string prefix, std::string sunFile, std::string spacecraftFile,
                                       Vector3 sunPosition, Vector3 spacecraftPosition);
    
    std::string getFirstElevenCharsFromFileName(std::string fileName);
    
    void indexFilesByKey(std::string dirName, std::map<std::string, std::string> & index);
    
  template <class pixelInType, class pixelOutType>
  bool getSubImageWithMargin(// Inputs
                             Vector2 begLonLat, Vector2 endLonLat,
                             std::string imageFile,
                             // Outputs
                             ImageView<pixelOutType>   & subImage,
                             cartography::GeoReference & sub_geo){
    
    // Read from disk only the portion of image whose pixel lon lat
    // values are in between begLonLat and endLonLat.  Adjust the
    // georeference accordingly. This is necessary to save on memory
    // for large images.
    
    // IMPORTANT: We assume that the input image is of pixelInType, and
    // we will convert the portion we read into pixelOutType.  Also we
    // read a few more pixels on each side, to help later with
    // interpolation.
  
    int extra = 2;
    DiskImageView<pixelInType> input_img(imageFile);
    //std::cout << "Reading: " << imageFile << std::endl;
    
    cartography::GeoReference input_geo;
    read_georeference(input_geo, imageFile);

    Vector2 begPix = input_geo.lonlat_to_pixel(begLonLat);
    Vector2 endPix = input_geo.lonlat_to_pixel(endLonLat);

    int beg_col = std::max(0,                (int)floor(begPix(0)) - extra);
    int end_col = std::min(input_img.cols(), (int)ceil(endPix(0))  + extra);
    int beg_row = std::max(0,                (int)floor(begPix(1)) - extra);
    int end_row = std::min(input_img.rows(), (int)ceil(endPix(1))  + extra);
  
    if (beg_col >= end_col || beg_row >= end_row) return false;
  
    subImage.set_size(end_col - beg_col, end_row - beg_row);
    for (int row = beg_row; row < end_row; row++){
      for (int col = beg_col; col < end_col; col++){
        // Note the pixelInType to pixelOutType conversion below
        subImage(col - beg_col, row - beg_row) = input_img(col, row); 
      }
    }
  
    // In sub_geo, the (0, 0) pixel will where (beg_col, beg_row) is in input_geo.
    sub_geo = vw::cartography::crop(input_geo, beg_col, beg_row);

    //system("echo getSubImageWithMargin top is $(top -u $(whoami) -b -n 1|grep lt-reconstruct)");
  
    return true;
  }

  template<class T>
  void dumpImageToFile(T & img, std::string fileName){
    
    printf("dumping %s\n", fileName.c_str());
    
    std::ofstream fs(fileName.c_str());
    fs.precision(20);
    
    for (int k = 0 ; k < img.rows(); ++k) {
      for (int l = 0; l < img.cols(); ++l) {
        fs << (double)img(l, k) << " ";
      }
      fs << std::endl;
    }
    fs.close();
  }

}} // end vw::photometry

#endif//__VW_PHOTOMETRY_MISC_H__
