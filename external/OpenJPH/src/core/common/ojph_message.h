//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_message.h
// Author: Aous Naman
// Date: 29 August 2019
//***************************************************************************/

#ifndef OJPH_MESSAGE_H
#define OJPH_MESSAGE_H 

#include <cstring>
#include "ojph_arch.h"

namespace ojph {

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief This enum is use to specify the level of severity of message while 
   *        processing markers
   * 
   */
  enum OJPH_MSG_LEVEL : int
  {
    ALL_MSG = 0,  // uninitialized or print all message
    INFO = 1,     // info message
    WARN = 2,     // warning message
    ERROR = 3,    // error message (the highest severity)
    NO_MSG = 4,   // no message (higher severity for message printing only)
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   *  @remark
   *   There are 3 levels of messaging; they are in order of level of 
   *   severity: INFO, WARNING, and ERROR.  ERROR is the most severe and 
   *   code execution must be terminated.
   * 
   *  @remark  
   *   The library provides two way to customize the reporting associated with 
   *   each messaging level:
   *   1. Calling set_XXXX_stream; this sets the library's output file stream 
   *      to a user defined stream, such as std_err or a log file; it can 
   *      also be set to NULL to prevent reporting.
   *   2. Calling configure_XXXX to pass a pointer to an object from a class 
   *      derived from the corresponding message_XXXX class. The derived 
   *      class must override the virtual operator() to perform the desired 
   *      behaviour.  Remember for message_error, the user must throw an 
   *      exception at the end of the implementation of operator().
   * 
   *   The customization is global, and cannot be separately tailored for
   *   each decoder's instantiation.
   */

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief This is the base class from which all messaging levels are derived
   * 
   *  Importantly it defined the base virtual operator() that must be defined
   *  in all derived classes.
   */
  class OJPH_EXPORT message_base {
  public:
    /**
     * @brief Prints a message and for errors throws an exception. 
     *        All derived classes must override this virtual function.
     * 
     * @param warn_code Message code (integer) for identifications.
     * @param file_name The file name where the message originates.
     * @param line_num  The line number where the message originates.
     * @param fmt       The format of the message; this is printf format.
     * @param ...       A variable number of parameters to print.  This is
     *                  the parameters you would pass to printf.
     */
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char *fmt, ...) = 0;
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Derived from message_base to handle info messages
   */
  class OJPH_EXPORT message_info : public message_base
  {
    public:
      /**
       * @brief See the base message_base::operator() for details about 
       *        parameters
       */
      virtual void operator() (int info_code, const char* file_name,
        int line_num, const char* fmt, ...);
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Replaces the info output file from the default stdout to user 
   *        defined output file.
   * 
   * @param s A pointer to the desired output file; it can be stdout, stderr,
   *          a log file, or NULL if no info messages are desired.
   */
  OJPH_EXPORT
    void set_info_stream(FILE* s);

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief This overrides the default behaviour of handling info messages.
   * 
   * @param info An object derived from message_info to implement the desired
   *             behaviour.
   */
  OJPH_EXPORT
    void configure_info(message_info* info);

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Get the info message object, whose operator() member class is 
   *        called for info messages -- See the macros below.
   * 
   * @return message_info* returns the active message_info object, or an object 
   *         of the message_info-derived class if one was set.  This object 
   *         handles info messages.  This is mainly to be used with the macros
   *         below.
   */
  OJPH_EXPORT
    message_info* get_info();

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Derived from message_base to handle warning messages
   */
  class OJPH_EXPORT message_warning : public message_base
  {
    public:
      /**
       * @brief See the base message_base::operator() for details about 
       *        parameters
       */
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char* fmt, ...);
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Replaces the warning output file from the default stdout to user 
   *        defined output file.
   * 
   * @param s A pointer to the desired output file; it can be stdout, stderr,
   *          a log file, or NULL if no warning messages are desired.
   */
  OJPH_EXPORT
    void set_warning_stream(FILE* s);

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief This overrides the default behaviour of handling warning messages.
   * 
   * @param warn An object derived from message_warning to implement the 
   *             desired behaviour.
   */
  OJPH_EXPORT
    void configure_warning(message_warning* warn);

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Get the warning message object, whose operator() member class is 
   *        called for warning messages -- See the macros below.
   * 
   * @return message_warning* returns the active message_warning object, or an
   *         object of the message_warning-derived class if one was set.  This 
   *         object handles warning messages.  This is mainly to be used with 
   *         the macros below.
   */
  OJPH_EXPORT
    message_warning* get_warning();

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Derived from message_base to handle error messages
   */
  class OJPH_EXPORT message_error : public message_base
  {
    public:
      /**
       * @brief See the base message_base::operator() for details about 
       *        parameters
       */
      virtual void operator() (int warn_code, const char* file_name,
        int line_num, const char *fmt, ...);
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Replaces the error output file from the default stderr to user 
   *        defined output file.
   * 
   * @param s A pointer to the desired output file; it can be stdout, stderr,
   *          a log file, or NULL if no error messages are desired.
   */
  OJPH_EXPORT
    void set_error_stream(FILE *s);
  
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief This overrides the default behaviour of handling error messages.
   * 
   * @param error An object derived from message_error to implement the 
   *              desired behaviour.  Remember, remember to throw an exception
   *              at the end.
   */
  OJPH_EXPORT
    void configure_error(message_error* error);
  
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Get the error message object, whose operator() member class is 
   *        called for error messages -- See the macros below.
   * 
   * @return message_error* returns the active message_error object, or an
   *         object of the message_error-derived class if one was set.  This 
   *         object handles error messages.  This is mainly to be used with 
   *         the macros below.
   */
  OJPH_EXPORT
    message_error* get_error();

  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Sets the minimum severity of the message to be reported.
   *
   * @param level is the level of the message severity; values are defined in
   *              OJPH_MSG_LEVEL.
   */
  OJPH_EXPORT
    void set_message_level(OJPH_MSG_LEVEL level);
}

//////////////////////////////////////////////////////////////////////////////
/**
 * @brief MACROS to remove the directory name from the file name
 */
#if (defined OJPH_OS_WINDOWS)
  #define __OJPHFILE__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
  #define __OJPHFILE__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

//////////////////////////////////////////////////////////////////////////////
/**
 * @brief MACROs to insert file and line number for info, warning, and error
 */
#define OJPH_INFO(t, ...) \
  { ojph::get_info()[0](t, __OJPHFILE__, __LINE__, __VA_ARGS__); }
#define OJPH_WARN(t, ...) \
  { ojph::get_warning()[0](t, __OJPHFILE__, __LINE__, __VA_ARGS__); }
#define OJPH_ERROR(t, ...) \
  { ojph::get_error()[0](t, __OJPHFILE__, __LINE__,__VA_ARGS__); }


#endif // !OJPH_MESSAGE_H
