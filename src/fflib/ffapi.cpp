#ifdef WITH_PETSC
#include <petsc.h>
#endif

/// \file
/// ======================================================================
/// Written by Antoine Le Hyaric
/// Laboratoire Jacques-Louis Lions
/// Université Pierre et Marie Curie-Paris6, UMR 7598, Paris, F-75005 France
/// http://www.ljll.math.upmc.fr/lehyaric
/// ======================================================================
/// This file is part of Freefem++
/// 
/// Freefem++ is free software; you can redistribute it and/or modify
/// it under the terms of the GNU Lesser General Public License as
/// published by the Free Software Foundation; either version 2.1 of
/// the License, or (at your option) any later version.
/// 
/// Freefem++  is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Lesser General Public License for more details.
/// 
/// You should have received a copy of the GNU Lesser General Public
/// License along with Freefem++; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
/// 02110-1301 USA
/// ======================================================================
/// headeralh cpp freefem start=21/01/10 upmc

// Proposed FreeFem++ Application Programming Interface
// ----------------------------------------------------

// headerfilter
#include "ffapi.hpp"
#ifdef FFLANG
#include "socket.hpp"
#include "spawn.hpp"
#include "buffer.hpp"
#endif
#include <iostream>
#include <cstdio>
#ifndef FFLANG
#include <cstdio>
#endif
#ifdef FFLANG
#include "options.hpp"
#include <stdlib.h>
#endif
#ifndef FFLANG
#ifdef _WIN32
#include <fcntl.h>
#endif
#endif
#ifndef FFLANG
#ifdef PARALLELE
#include "mpi.h"
#endif
#endif
// Add dec 2014
#include <vector>
typedef void (*AtEnd)();
vector<AtEnd> AtFFEnd;
void ff_finalize()
{
    for (vector<AtEnd>::const_reverse_iterator i=AtFFEnd.rbegin(); i !=AtFFEnd.rend(); ++ i)
        (**i)();
    AtFFEnd.clear(); 
}
void ff_atend(AtEnd f)
{
    AtFFEnd.push_back(f);
}

// FFCS-specific implementations for the FF API
// --------------------------------------------

/// FFLANG defined means that FFCS is being compiled. I am fairly confident that FFCS will not be defined while
/// compiling the original FF.

/// Need to choose a non-zero stream number because FF will check it (as global variable ThePlotStream)
#define FFAPISTREAM 1

/// if FFCS is around, we need to bufferize all communications to avoid mixing up CMD_FFG and CMD_STDOUT messages
#ifdef FFLANG
void bufferwrite(const char *b,const int l){
  Socket::dialoglock->WAIT(); // need #include "socket.hpp"

  // thank to the buffering, there is only one CMD_FFG tag for multiple visualization data items.
  onlyffsock()<<CMD_FFG; // need #include "spawn.hpp"
  onlyffsock()<<l;

  // this call contains the socket MAGIC number
  onlyffsock().bufferedwrite(static_cast<const char*>(b),l);

  Socket::dialoglock->Free();
}

Buffer buffer(NULL,bufferwrite); // need #include "buffer.hpp"
#endif

namespace ffapi{

  // Get a pointer to the local cin/cout (which is distinct from ffcs's stdin/stdout under Windows because each DLL owns
  // separate cin/cout objects).

  // need #include <iostream>
  std::istream *cin(){return &std::cin;}
  std::ostream *cout(){return &std::cout;}
  std::ostream *cerr(){return &std::cerr;}

  // FFCS - ::stdout not accepted under mingw32
  // need #include <cstdio>
  FILE *ffstdout(){return stdout;}
  FILE *ffstderr(){return stderr;}
  FILE *ffstdin(){return stdin;}

  void newplot(){}

  FILE *ff_popen(const char *command, const char *type){
#ifdef FFLANG

    // this happens right at the begining of FF, so the socket
    // communication must not be started yet (only when actual
    // visualization data needs to be transfered).

    PROGRESS;
    return (FILE*)FFAPISTREAM;
#else

    // need #include <cstdio>
    return popen(command,type);
#endif
  }

  int ff_pclose(FILE *stream){
#ifdef FFLANG
    // nothing to close in FFCS
    return 0;
#else
    return pclose(stream);
#endif
  }

  size_t fwriteinit(const void *ptr, size_t size, size_t nmemb,FILE *stream){

    // printf() is useful for debug because it is not redirected through
    // the FFCS socket. But it is asynchronous with cout so it may end up
    // in the middle of the lines checked by test/compare. So deactivate
    // it by default.
#ifdef DEBUG_FFAPI
#ifdef FFLANG
    printf("debug: ffapi: using TCP sockets\n");
#else
    printf("debug: ffapi: using an anonymous pipe\n");
#endif // FFLANG
#endif // DEBUG_FFAPI

#ifdef FFLANG

    // Ask FFCS to analyze the visualization flux header. I could just skip this stage, but it will be useful to check
    // the coherency between FFCS and FF when FF evolves in the future.

    Socket::dialoglock->WAIT();
    onlyffsock()<<CMD_FFGINIT;
    Socket::dialoglock->Free();
#endif
    return ff_fwrite(ptr,size,nmemb,stream);
  }

  size_t ff_fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream){
#ifdef FFLANG

    // if the ffsock pointer is null here, it means that the pointer exported from the FFCS shared library is not a
    // valid one (this has been the case with Windows DLLs in the past).

    // we won't make use of the stream, but make sure that the call from
    // FF is coherent with what we know.
    assert(stream==(FILE*)FFAPISTREAM);

    buffer.write(static_cast<const char*>(ptr),size*nmemb);

    // stops the server flux at one precise point (point value expressed during a previous crash while reading server
    // data in the client in visudata.cpp). Use abort() to call the debugger (which can display the call stack and show
    // where the problematic pipe value came from).

    // need #include "options.hpp"
    if(options->AbortFFGraphicsDataAt==buffer.getpoint())abort(); // need #include <stdlib.h>

#else
    fwrite(ptr,size,nmemb,stream);
#endif
    return 0;
  }

  int ff_fflush(FILE *stream){
#ifdef FFLANG
    assert(stream==(FILE*)FFAPISTREAM);

    // we need to flush both the buffer and the socket to avoid a separate callback for flush in the buffer
    buffer.flush();

    // ff_fflush() is used by FF only at the end of a plot, so we can use this location to send a marker for the
    // sequential java client to deal with one complete plot at a time.
    Socket::dialoglock->WAIT();
    onlyffsock()<<CMD_FFGEND;
    onlyffsock().writeflush();
    Socket::dialoglock->Free();

#else
    fflush(stream);
#endif
    return 0;
  }

  int ff_ferror(FILE *stream){
#ifndef FFLANG
    return ferror(stream);
#else
    return 0;
#endif
  }

  int ff_feof(FILE *stream){
#ifndef FFLANG
    return feof(stream);
#else
    return 0;
#endif
  }

  void wintextmode(FILE *f){
#ifndef FFLANG
#ifdef _WIN32
    // need #include <fcntl.h>
    _setmode(fileno(f),O_TEXT);	
#endif
#endif
  }

  void winbinmode(FILE *f){
#ifndef FFLANG
#ifdef _WIN32
    _setmode(fileno(f),O_BINARY);	
#endif
#endif
  }

  void mpi_init(int &argc, char** &argv){
    /// only call MPI_Init() if this has not already been done in ffcs/src/server.cpp
#ifndef FFLANG
#ifdef PARALLELE
    // need #include "mpi.h"
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if(provided < MPI_THREAD_SERIALIZED) {
        MPI_Comm_rank(MPI_COMM_WORLD, &provided);
        if(provided == 0)
            std::cout << "MPI_THREAD_SERIALIZED not supported !" << std::endl;
    }
#ifdef WITH_PETSCxxxxx
    PetscInitialize(&argc, &argv, 0, "");
#endif

#endif
#endif
  }

  void mpi_finalize(){
#ifndef FFLANG
#ifdef PARALLELE
#ifdef WITH_PETSCxxxxxxxx
    PetscFinalize();
#endif
    MPI_Finalize();
#endif
#endif
  }

  bool protectedservermode(){
#ifdef FFLANG
    return !options->LocalClient;
#else
    return false;
#endif
  }

  void init(){
    ffapi::fwriteinit;
    ffapi::winbinmode;
    ffapi::wintextmode;
    ffapi::mpi_finalize;
    ffapi::cin;
    ffapi::cerr;
    ffapi::cout;
    ffapi::ff_feof;
    ffapi::newplot;
    ffapi::ff_popen;
    ffapi::mpi_init;
    ffapi::ff_ferror;
    ffapi::ff_fflush;
    ffapi::ff_fwrite;
    ffapi::ff_pclose;
    ffapi::protectedservermode;
  }
}

/// Local Variables:
/// mode:c++
/// ispell-local-dictionary:"british"
/// coding:utf-8
/// End:
