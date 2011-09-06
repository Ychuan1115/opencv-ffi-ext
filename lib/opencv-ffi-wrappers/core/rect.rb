
require 'opencv-ffi-wrappers/core'

module CVFFI

  class Rect
    attr_accessor :size :origin

    def initialize( args )
      case args.class
      when Array
        @origin = Point.new( args[0..1] )
        @size   = Size.new(  arg[2..3] )
      when Hash
        if args[:origin] 
          @origin = Point.new( args[:origin] )
        else
          @origin = Point.new(args)
          args.delete :x
          args.delete :y
        end

        if args[:size]
          @size = Size.new args[:size]
        else
          @size = Size.new(args)
        end
      else
        @size = Size.new args.size
        @origin = Point.new args.origin
      end
    end

    def to_CvRect
      CvRect( :x => @origin.x, :y => @origin.y,
              :width => @size.width, :height => @size.height )
    end
  end
end