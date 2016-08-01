/*******************************************************************************
* ALMA - Atacama Large Millimiter Array
* (c) Associated Universities Inc., 2010
*
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Lesser General Public
*License as published by the Free Software Foundation; either
*version 2.1 of the License, or (at your option) any later version.
*
*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Lesser General Public License for more details.
*
*You should have received a copy of the GNU Lesser General Public
*License along with this library; if not, write to the Free Software
*Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/

#ifndef BAND7_H_
#define BAND7_H_

#include "ColdCartridgeTestFixture.h"
#include "WCATestFixture.h"

class Band7CCATestFixture : public ColdCartridgeTestFixture {
public:
    Band7CCATestFixture()
      : ColdCartridgeTestFixture(7, "Band7CCATestFixture")
        {}
protected:
    virtual const std::vector<float> &getTestValuesSISVoltage() const;
    ///< Get the band-specific test values for SIS voltage.

    virtual void getLimitsSisVoltage(float &min, float &max, float &tolerance) const;
    ///< Get the band-specific limits and tolerance for SIS voltage.

    virtual void getLimitsSISCurrent(float &min, float &max) const;
    ///< Get the band-specific limits for SIS current.

    virtual const std::vector<float> &getTestValuesMagnetCurrent() const;
    ///< Get the band-specific test values for SIS magnet current.

    virtual void getLimitsMagnetCurrent(float &min, float &max, float &tolerance) const;
    ///< Get the band-specific limits for magnet current.

    virtual void getLimitsMagnetVoltage(float &min, float &max) const;
    ///< Get the band-specific limits for magnet voltage.

    virtual void getLimitsHeaterCurrent(float &min, float &max) const;
    ///< Get the band-specific limits for heater current.

private:
    INSTANTIATE_COLDCART_SUITE(Band7CCATestFixture)
};

class Band7WCATestFixture : public WCATestFixture {
public:
    Band7WCATestFixture()
      : WCATestFixture(7, "Band7WCATestFixture")
        {}
private:
    INSTANTIATE_WCA_SUITE(Band7WCATestFixture)
};

#endif /* BAND7_H_ */
