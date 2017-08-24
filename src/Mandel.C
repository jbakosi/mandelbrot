// *****************************************************************************
/*!
  \file      src/Mandel.C
  \copyright 2016-2017, Los Alamos National Security, LLC.
  \brief     Mandel's Charm++ main chare and main().
  \details   Mandel's Charm++ main chare and main(). This file contains
    the definition of the Charm++ main chare, equivalent to main() in Charm++-
    land.
*/
// *****************************************************************************

#include "NoWarning_gil.h"

#include "NoWarning_mandel.decl.h"

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

//! \brief Charm handle to the main proxy, facilitates call-back to finalize,
//!    etc., must be in global scope, unique per executable
CProxy_Main mainProxy;

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

using namespace boost::gil;

// Models a Unary Function
template <typename P>   // Models PixelValueConcept
struct mandelbrot_fn {
    using point_t = point2< ptrdiff_t >;

    using const_t = mandelbrot_fn;
    using value_type = P;
    using reference = value_type;
    using const_reference = value_type;
    using argument_type = point_t;
    using result_type = reference;
    BOOST_STATIC_CONSTANT(bool, is_mutable=false);

    value_type                    _in_color,_out_color;
    point_t                       _img_size;
    static const int MAX_ITER=100;        // max number of iterations

    mandelbrot_fn() {}
    mandelbrot_fn( const point_t& sz,
                   const value_type& in_color,
                   const value_type& out_color ) :
      _in_color(in_color), _out_color(out_color), _img_size(sz) {}

    result_type operator()(const point_t& p) const {
        // normalize the coords to (-2..1, -1.5..1.5)
        // (actually make y -1.0..2 so it is asymmetric, so we can verify some
        // view factory methods)
        double t = get_num_iter(
          point2<double>( p.x/static_cast<double>(_img_size.x)*3.0-2,
                          p.y/static_cast<double>(_img_size.y)*3.0-1.5 ) );
        t = std::pow( t, 0.2 );

        value_type ret;
        for (std::size_t k=0; k<num_channels<P>::value; ++k)
            ret[k] = static_cast< typename channel_type<P>::type >(
                     (_in_color[k]*t + _out_color[k]*(1-t)) );
        return ret;
    }

private:
    double get_num_iter(const point2<double>& p) const {
        point2<double> Z(0,0);
        for (int i=0; i<MAX_ITER; ++i) {
            Z = point2<double>(Z.x*Z.x - Z.y*Z.y + p.x, 2*Z.x*Z.y + p.y);
            if (Z.x*Z.x + Z.y*Z.y > 4)
                return i/static_cast<double>(MAX_ITER);
        }
        return 0;
    }
};

class Main : public CBase_Main {

  private:
          unsigned long int start_s;
          unsigned long int stop_s;

  public:
    Main( CkArgMsg* msg ) {

      // initialize state
      start_s = clock();
      int numPixel;
      double virtualization;

      // set # of pixels
      if (msg->argc>1)
      {
              numPixel = atoi(msg->argv[1]);
      }
      else
      {
              numPixel = 600;
      }

      // set degree of virtualization
      if (msg->argc>2)
      {
              virtualization = atof(msg->argv[2]);
      }
      else
      {
              virtualization = 0.0;
      }

      // screen outputs about the code
      CkPrintf("\n ------------------------------------------------------ \n");
      CkPrintf(" Number of Pixels: %d \n", numPixel);
      CkPrintf(" Number of PE's  : %d \n", CkNumPes());
      CkPrintf(" Virtualization  : %f \n", virtualization);
      CkPrintf(" ------------------------------------------------------ \n \n");

      delete msg;
      using deref_t = mandelbrot_fn< rgb8_pixel_t >;
      using point_t = deref_t::point_t;
      using locator_t = virtual_2d_locator< deref_t, false >;
      using my_virt_view_t = image_view< locator_t >;

      boost::function_requires< PixelLocatorConcept<locator_t> >();
      gil_function_requires< StepIteratorConcept< locator_t::x_iterator > >();

      point_t dims( numPixel, numPixel );
      my_virt_view_t mandel(dims, locator_t(point_t(0,0), point_t(1,1),
        deref_t(dims, rgb8_pixel_t(255,0,255), rgb8_pixel_t(0,255,0))));
      jpeg_write_view("out-mandelbrot.jpg",mandel);

      CkExit();
    }

    // reduction to ensure completion and then exit
    void complete()
    {
      stop_s = clock();
      CkPrintf(" Computation time: %f . \n", (stop_s-start_s)/double(CLOCKS_PER_SEC));
      CkPrintf(" ------------------------------------------------------ \n \n");
      CkExit();
    }
};

#include "NoWarning_mandel.def.h"
