
require 'nice-ffi'

require 'opencv-ffi-wrappers'
require 'opencv-ffi-wrappers/misc/params'

require 'opencv-ffi-ext/features2d/keypoint'

module CVFFI

  module Features2D
    module HarrisCommon

      class HarrisLaplaceParams < NiceFFI::Struct
        layout :octaves, :int,
          :quality_level, :float,
          :dog_thresh, :float,
          :max_corners, :int,
          :num_layers, :int,
          :harris_k, :float
      end

      class Params < CVFFI::Params
        param :octaves, 6
        param :quality_level, 0.01
        param :dog_thresh, 0.01
        param :max_corners, 0
        param :num_layers, 4
        param :harris_k, 0.04

        def to_HarrisLaplaceParams
          HarrisLaplaceParams.new( @params  )
        end
      end

      def detect( image, params = HarrisCommon::Params.new )
        params = params.to_HarrisLaplaceParams unless params.is_a?( HarrisLaplaceParams )

        kp_ptr = FFI::MemoryPointer.new :pointer
        storage = CVFFI::cvCreateMemStorage( 0 )

        image = image.ensure_greyscale

        seq_ptr = actual_detector( image, storage, params )

        keypoints = CVFFI::CvSeq.new( seq_ptr )
        #puts "Returned #{keypoints.total} keypoints"

        wrap_output( keypoints, storage )
      end
 
    end

    module HarrisLaplace
      extend NiceFFI::Library
      libs_dir = File.dirname(__FILE__) + "/../../../ext/opencv-ffi/"
      pathset = NiceFFI::PathSet::DEFAULT.prepend( libs_dir )
      load_library("cvffi", pathset)

      extend HarrisCommon

      attach_function :cvHarrisLaplaceDetector, [:pointer, :pointer, HarrisCommon::HarrisLaplaceParams.by_value ], CvSeq.typed_pointer

      def self.actual_detector( *args ); cvHarrisLaplaceDetector( *args ); end
      def self.wrap_output( *args ); Keypoints.new( *args ); end
   end

    module HarrisAffine
      extend NiceFFI::Library
      libs_dir = File.dirname(__FILE__) + "/../../../ext/opencv-ffi/"
      pathset = NiceFFI::PathSet::DEFAULT.prepend( libs_dir )
      load_library("cvffi", pathset)

      extend HarrisCommon

      attach_function :cvHarrisAffineDetector, [:pointer, :pointer, HarrisCommon::HarrisLaplaceParams.by_value ], CvSeq.typed_pointer

      def self.actual_detector( *args ); cvHarrisAffineDetector( *args ); end
      def self.wrap_output( *args ); EllipticKeypoints.new( *args ); end

    end
  end
end
