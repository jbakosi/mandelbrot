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

#include <limits>
#include <cstdint>


// *****************************************************************************
#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

//! \brief Charm handle to the main proxy, facilitates call-back to finalize,
//!    etc., must be in global scope, unique per executable
CProxy_Main mainProxy;
int numchare;

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

class Main : public CBase_Main {

  private:
    uint64_t start_s, stop_s;

    uint64_t
    linearLoadDistributor( double virtualization,
                           uint64_t load,
                           int npe,
                           uint64_t& chunksize,
                           uint64_t& remainder )
    // *****************************************************************************
    //  Compute linear load distribution for given total work and virtualization
    //! \param[in] virtualization Degree of virtualization [0.0...1.0]
    //! \param[in] load Total load, e.g., number of particles, number of mesh cells
    //! \param[in] npe Number of processing elements to distribute the load to
    //! \param[inout] chunksize Chunk size, see detailed description
    //! \param[inout] remainder Remainder, see detailed description
    //! \return Number of work units
    //! \details Compute load distibution (number of chares and chunksize) based on
    //!   total work (e.g., total number of particles) and virtualization
    //!
    //!   The virtualization parameter, specified by the user, is a real number
    //!   between 0.0 and 1.0, inclusive, which controls the degree of
    //!   virtualization or over-decomposition. Independent of the value of
    //!   virtualization the work is approximately evenly distributed among the
    //!   available processing elements, given by npe. For zero virtualization (no
    //!   over-decomposition), the work is simply decomposed into total_work/numPEs,
    //!   which yields the smallest number of Charm++ chares and the largest chunks
    //!   of work units. The other extreme is unity virtualization, which decomposes
    //!   the total work into the smallest size work units possible, yielding the
    //!   largest number of Charm++ chares. Obviously, the optimum will be between
    //!   0.0 and 1.0, depending on the problem.
    //!
    //!   The formula implemented uses a linear relationship between the
    //!   virtualization parameter and the number of work units with the extremes
    //!   described above. The formula is given by
    //!
    //!   chunksize = (1 - n) * v + n;
    //!
    //!   where
    //!    - v = degree of virtualization
    //!    - n = load/npes
    //!    - load = total work, e.g., number of particles, number of mesh cells
    //!    - npes = number of hardware processing elements
    //! \author J. Bakosi
    // *****************************************************************************
    {
      //Assert( virtualization > -std::numeric_limits< double >::epsilon() &&
      //        virtualization < 1.0+std::numeric_limits< double >::epsilon(),
      //        "Virtualization parameter must be between [0.0...1.0]" );
      //Assert( npe > 0, "Number of processing elements must be larger than zero" );

      // Compute minimum number of work units
      const auto n = static_cast< double >( load ) / npe;

      // Compute work unit size based on the linear formula above
      chunksize = static_cast< uint64_t >( (1.0 - n) * virtualization + n );

      // Compute number of work units with size computed ignoring remainder
      uint64_t nchare = load / chunksize;

      // Compute remainder of work if the above number of units were to be created
      remainder = load - nchare * chunksize;

      // Redistribute remainder among the work units for a more equal distribution
      chunksize += remainder / nchare;

      // Compute new remainder (after redistribution of the previous remainder)
      remainder = load - nchare * chunksize;

      // Return number of work units (number of Charm++ chares)
      return nchare;
    }


  public:
    Main( CkArgMsg* msg ) {

      // initialize state
      start_s = clock();
      mainProxy = thisProxy;
      int imgsize;
      double virtualization;

      // set # of pixels
      if (msg->argc>1)
      {
              imgsize = atoi(msg->argv[1]);
      }
      else
      {
              imgsize = 600;
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

      delete msg;

      uint64_t chunksize, remainder;

      numchare = linearLoadDistributor( virtualization,
                                        imgsize,
                                        CkNumPes(),
                                        chunksize,
                                        remainder );

      // screen outputs about the code
      CkPrintf("\n ------------------------------------------------------\n");
      CkPrintf(" Width in Pixels  : %d \n", imgsize);
      CkPrintf(" Number of PEs    : %d \n", CkNumPes());
      CkPrintf(" Virtualization   : %f \n", virtualization);
      CkPrintf(" Chunksize        : %d \n", chunksize);
      CkPrintf(" Remainder        : %d \n", remainder);
      CkPrintf(" Load distribution: %d (%d*%d+%d)\n",
                 numchare, numchare-1, chunksize, chunksize+remainder );

      CkPrintf(" ------------------------------------------------------\n");

      // create the chareArray
      CProxy_mandelChare mandelArray = CProxy_mandelChare::ckNew(numchare);

      // compute Mandelbrot set in parallel
      mandelArray.compute(imgsize,chunksize,remainder);
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


// *****************************************************************************
class mandelChare : public CBase_mandelChare
{
        private:

        // class that generates pixels of Mandelbrot set
        template <typename P>   // Models PixelValueConcept
        struct mandelbrot_fn {

            template< typename T > using point2 = boost::gil::point2< T >;
            using num_channels = typename boost::gil::num_channels< P >;
            using channel_type = typename boost::gil::channel_type< P >::type;

            using point_t = point2< ptrdiff_t >;

            using const_t = mandelbrot_fn;
            using value_type = P;
            using reference = value_type;
            using const_reference = value_type;
            using argument_type = point_t;
            using result_type = reference;

            value_type                    _in_color,_out_color;
            point_t                       _img_size;
            static const int MAX_ITER=100;        // max number of iterations

            mandelbrot_fn( const point_t& sz,
                           const value_type& in_color,
                           const value_type& out_color ) :
              _in_color(in_color), _out_color(out_color), _img_size(sz) {}

            result_type operator()(const point_t& p) const {

                double t = get_num_iter(
                  point2<double>( p.x/static_cast<double>(numchare*_img_size.x),
                                  p.y/static_cast<double>(_img_size.y) ) );
                t = std::pow( t, 0.2 );

                value_type ret;
                for (std::size_t k=0; k<num_channels::value; ++k)
                    ret[k] = static_cast< channel_type >(
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

        public:

        // migrate constructor
        mandelChare(CkMigrateMessage* msg) { delete msg; }

        // constructor
        mandelChare() {}

        void compute(int imgsize, uint64_t chunksize, uint64_t remainder)
        {
                auto width = chunksize;
                if (CkMyPe() == CkNumPes()-1) width += remainder;

                int x = -imgsize*2 + thisIndex*4*width;
                int y = -imgsize*2;

                CkPrintf("%d: startx: %d, width: %d\n",thisIndex,x,4*width);

                using rgb8_pixel_t = boost::gil::rgb8_pixel_t;
                using deref_t = mandelbrot_fn< rgb8_pixel_t >;
                using point_t = deref_t::point_t;
                using locator_t = boost::gil::virtual_2d_locator< deref_t, false >;
                using my_virt_view_t = boost::gil::image_view< locator_t >;

                boost::function_requires< boost::gil::PixelLocatorConcept<locator_t> >();
                boost::gil::gil_function_requires<
                  boost::gil::StepIteratorConcept< locator_t::x_iterator > >();

                point_t dims( width, imgsize );
                my_virt_view_t mandel(dims, locator_t(point_t(x,y), point_t(4,4),
                  deref_t(dims, rgb8_pixel_t(0,0,0), rgb8_pixel_t(0,255,0))));

                std::string filename, fileIndex;
                fileIndex = std::to_string(thisIndex);
                filename = fileIndex + ".mandelbrot.jpg";
                jpeg_write_view(filename,mandel);

                // signal the runtime system that we are done with our part
                contribute( CkCallback(CkReductionTarget(Main,complete), mainProxy) );
        }
};

#include "NoWarning_mandel.def.h"
