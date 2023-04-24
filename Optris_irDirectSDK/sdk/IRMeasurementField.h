#pragma once

#include "irdirectsdk_defs.h"
#include <stdlib.h>
#include <string.h>

namespace evo
{

class IRMeasurementField;

/**
 * Callback type for measurement fields.
 * @param[in] field measurement field
 */
typedef void(__CALLCONV *fptrIRMeasurementField)(IRMeasurementField* field);

/**
 * @class IRMeasurementField
 * @brief Measurement field having individual radiation parameters
 * @author Stefan May (Evocortex GmbH)
 */
class IRMeasurementField
{
public:
  /**
   * Constructor
   * @param[in] id identifier, use a unique number to make your measurement fields identifiable
   * @param[in] x x-coordinate
   * @param[in] y y-coordinate
   * @param[in] w width
   * @param[in] h height
   * @param[in] emissivity emissivity parameter of material to be monitored
   * @param[in] transmissivity transmissivity of material in between camera and object, i.e. the transmissivity of the protective glass, if used.
   * @param[in] callback callback function to be called within process method of IRImager
   * @param[in] arg pointer to user-defined data being passed to callback function
   */
  IRMeasurementField(unsigned int id, unsigned int x, unsigned int y, unsigned int w, unsigned int h, float emissivity, float transmissivity, fptrIRMeasurementField callback, void* arg)
  {
    _id             = id;
    _x              = x;
    _y              = y;
    _w              = w;
    _h              = h;
    _emissivity     = emissivity;
    _transmissivity = transmissivity;
    _callback       = callback;
    _arg            = arg;
    _tMean          = 0.f;
    _tMin           = 0.f;
    _tMax           = 0.f;
    if(w*h>0)
    {
      _data = new unsigned short[w*h];
    }
    else
    {
      _data = NULL;
    }
  }

  /**
   * Copy constructor
   * @param[in] field instance to be copied
   */
  IRMeasurementField(IRMeasurementField* field)
  {
    _id             = field->_id;
    _x              = field->_x;
    _y              = field->_y;
    _w              = field->_w;
    _h              = field->_h;
    _emissivity     = field->_emissivity;
    _transmissivity = field->_transmissivity;
    _callback       = field->_callback;
    _arg            = field->_arg;
    _tMean          = field->_tMean;
    _tMin           = field->_tMin;
    _tMax           = field->_tMax;
    if(_w*_h>0)
    {
      _data = new unsigned short[_w*_h];
      memcpy(_data, field->_data, _w*_h*sizeof(unsigned short));
    }
    else
    {
      _data = NULL;
    }
  }

  /**
   * Destructor
   */
  ~IRMeasurementField()
  {
    if(_data)
      delete [] _data;
  }

  unsigned int           _id;
  unsigned int           _x;
  unsigned int           _y;
  unsigned int           _w;
  unsigned int           _h;
  float                  _emissivity;
  float                  _transmissivity;
  fptrIRMeasurementField _callback;
  void*                  _arg;
  unsigned short*        _data;
  float                  _tMean;
  float                  _tMin;
  float                  _tMax;
};

}
