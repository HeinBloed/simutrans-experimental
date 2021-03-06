/*
 * Copyright (c) 2009 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * convoy_t: common collection of properties of a convoi_t and a couple of vehicles which are going to become a convoy_t.
 * While convoi_t is involved in game play, convoy_t is the entity that physically models the convoy.
 */
/*******************************************************************************

Vehicle/Convoy Physics.

We know: delta_v = a * delta_t, where acceleration "a" is nearly constant for very small delta_t only.

At http://de.wikipedia.org/wiki/Fahrwiderstand we find a 
complete explanation of the force equation of a land vehicle.
(Sorry, there seems to be no english pendant at wikipedia).

Force balance: Fm = Ff + Fr + Fs + Fa; 

Fm: machine force in Newton [N] = [kg*m/s^2]
Ff: air resistance, always > 0
    Ff = cw/2 * A * rho * v^2, 
		cw: friction factor: average passenger cars and high speed trains: 0.25 - 0.5, average trucks and trains: 0.7
		A: largest profile: average passenger cars: 3, average trucks: 6, average train: 10 [m^2]
		rho = density of medium (air): 1.2 [kg/m^3]
		v: speed [m/s]
Fr: roll resistance, always > 0 
    Fr = fr * g * m * cos(alpha)
		fr: roll resistance factor: steel wheel on track: 0.0015, car wheel on road: 0.015
		g: gravitation constant: 9,81 [m/s^2]
		m: mass [kg]
		alpha: inclination: 0=flat
Fs: slope force/resistance, downhill: Fs < 0 (force), uphill: Fs > 0 (resistance)
	Fs = g * m * sin(alpha)
		g: gravitation constant: 9.81 [m/s^2]
		m: mass [kg]
		alpha: inclination: 0=flat
Fa: accelerating force
	Fa = m * a
		m: mass [kg]
		a: acceleration

Let F = Fm - Fr - Fs.
Let cf = cw/2 * A * rho.
Let Frs = Fr + Fs = g * (fr * m * cos(alpha) + m * sin(alpha))

Then

cf * v^2 + m * a - F = 0

a = (F - cf * v^2 - Frs) / m

*******************************************************************************/
#pragma once

#ifndef convoy_h
#define convoy_h

#include <limits>
#include <math.h>
#include "utils/float32e8_t.h"
#include "tpl/vector_tpl.h"
#include "besch/vehikel_besch.h"
//#include "simconst.h"
#include "simtypes.h"
#include "vehicle/simvehikel.h"
#include "simconvoi.h"
#include "simworld.h"

// CF_*: constants related to air resistance
// TODO: Add a "streamline" value to road/rail
// which reduces the CF value.

//#define CF_TRACK 0.7 / 2 * 10 * 1.2
//#define CF_TRACK 4.2
#define CF_TRACK float32e8_t((uint32) 13)
#define CF_MAGLEV float32e8_t((uint32) 10)
//#define CF_ROAD 0.7 / 2 * 6 * 1.2
#define CF_ROAD float32e8_t((uint32) 252, (uint32) 100)
#define CF_WATER float32e8_t((uint32) 25)
#define CF_AIR float32e8_t((uint32)1)

// FR_*: constants related to roll resistance

//should be 0.0015, but for game balance it is higher 
//#define FR_TRACK 0.0015
#define FR_MAGLEV float32e8_t((uint32) 15, (uint32) 10000)
#define FR_TRACK float32e8_t((uint32) 51, (uint32) 10000)
#define FR_ROAD  float32e8_t((uint32) 15, (uint32) 1000)
#define FR_WATER float32e8_t((uint32) 1, (uint32) 1000)
#define FR_AIR float32e8_t((uint32) 1, (uint32) 1000)

// GEAR_FACTOR: a gear of 1.0 is stored as 64
#define GEAR_FACTOR 64

#define WEIGHT_UNLIMITED ((std::numeric_limits<sint32>::max)())

// anything greater then 2097151 will give us overflow in kmh_to_speed. 
#define KMH_SPEED_UNLIMITED  (300000)


/**
 * Convert simutrans speed to m/s
 */

// scale to convert m/s to simutrans speed
const float32e8_t simspeed2ms((uint32) 10, (uint32) 36 * 1024);
const float32e8_t ms2simspeed((uint32) 36 * 1024, (uint32) 10);

inline float32e8_t speed_to_v(const sint32 speed)
{
	return simspeed2ms * (speed * VEHICLE_SPEED_FACTOR);
}

inline sint32 v_to_speed(const float32e8_t &v)
{
	return ((sint32)(ms2simspeed * v) + VEHICLE_SPEED_FACTOR - 1) / VEHICLE_SPEED_FACTOR;
}

//inline float32e8_t x_to_steps(const float32e8_t &x)
//{
//	return (ms2simspeed * x + float32e8_t(VEHICLE_SPEED_FACTOR - 1)) / float32e8_t(VEHICLE_SPEED_FACTOR);
//}

/******************************************************************************/

struct vehicle_summary_t
{
	uint32 length;			// sum of vehicles' length in 1/OBJECT_OFFSET_STEPSth of a tile
	uint32 tiles;           // length of convoy in tiles.
	sint32 weight;			// sum of vehicles' own weight without load in kg
	sint32 max_speed;		// minimum of all vehicles' maximum speed in km/h

	inline void clear()
	{
		length = 0;
		tiles = 0;
		weight = 0;
		max_speed = KMH_SPEED_UNLIMITED; // if there is no vehicle, there is no speed limit!
	}

	inline void add_vehicle(const vehikel_besch_t &b)
	{
		length += b.get_length();
		weight += b.get_gewicht();
		max_speed = min(max_speed, b.get_geschw());
	}

	// call update_summary() after all vehicles have been added.
	inline void update_summary(uint8 length_of_last_vehicle)
	{
		// this correction corresponds to the correction in convoi_t::get_tile_length()
		tiles = (length + (max(8, length_of_last_vehicle) - length_of_last_vehicle) + OBJECT_OFFSET_STEPS - 1) / OBJECT_OFFSET_STEPS;
		weight *= 1000;
	}
};

/******************************************************************************/

// should have been named: struct environ_summary_t
// but "environ" is the name of a defined macro. 
struct adverse_summary_t
{
	float32e8_t cf;	// air resistance constant: cf = cw/2 * A * rho. Depends on rho, which depends on altitude.
	float32e8_t fr;	// roll resistance: depends on way
	sint32 max_speed;

	inline void clear()
	{
		cf = fr = 0;
		max_speed = KMH_SPEED_UNLIMITED; 
	}

	inline void set_by_waytype(waytype_t waytype)
	{
		switch (waytype)
		{
			case air_wt:
				cf = CF_AIR;
				fr = FR_AIR;
				break;

			case water_wt:
				cf = CF_WATER;
				fr = FR_WATER;
				break;
		
			case track_wt:
			case overheadlines_wt: 
			case tram_wt:
			case narrowgauge_wt:
			case monorail_wt:      
				cf = CF_TRACK;
				fr = FR_TRACK;
				break;
			
			case maglev_wt:
				cf = CF_MAGLEV;
				fr = FR_MAGLEV;
				break;

			default:
				cf = CF_ROAD;
				fr = FR_ROAD;
				break;
		}
	}

	void add_vehicle(const vehikel_t &v);
};

/******************************************************************************/

struct freight_summary_t
{
	// several freight of the same category may weigh different: 
	sint32 min_freight_weight; // in kg
	sint32 max_freight_weight; // in kg

	inline void clear()
	{
		min_freight_weight = max_freight_weight = 0;
	}

	void add_vehicle(const vehikel_besch_t &b);
};

/******************************************************************************/

struct weight_summary_t
{
	sint32 weight;			// vehicle and freight weight in kg. depends on vehicle (weight) and freight (weight)
	float32e8_t weight_cos;		// vehicle and freight weight in kg multiplied by cos(alpha). depends on adverse (way/inclination), vehicle and freight
	float32e8_t weight_sin;		// vehicle and freight weight in kg multiplied by sin(alpha). depends on adverse (way/inclination), vehicle and freight

	weight_summary_t()
	{
	}

	weight_summary_t(sint32 kgs, sint32 sin_alpha)
	{
		clear();
		add_weight(kgs, sin_alpha);
	}

	inline void clear()
	{
		weight_cos = weight_sin = (uint32) 0;
		weight = 0;
	}

	/**
	 * kgs: weight in kilograms
	 * sin_alpha: inclination and friction factor == 1000 * sin(alpha), 
	 *		      e.g.: 50 corresponds to an inclination of 28 per mille.
	 */
	void add_weight(sint32 kgs, sint32 sin_alpha);

	inline void add_vehicle(const vehikel_t &v)
	{
		// v.get_frictionfactor() between about -14 (downhill) and 50 (uphill). 
		// Including the factor 1000 for tons to kg conversion, 50 corresponds to an inclination of 28 per mille.
		add_weight(v.get_gesamtgewicht() * 1000, v.get_frictionfactor());
	}
};

/******************************************************************************/

class convoy_t /*abstract */
{
private:
	/**
	 * Get force in N according to current speed in m/s
	 */
	inline sint32 get_force(float32e8_t speed) 
	{
		sint32 v = (sint32)abs(speed);
		return (v == 0) ? get_starting_force() : get_force_summary(v) * 1000;
	}
	/*
	 * Get force in N that holds the given speed v or maximum available force, what ever is lesser.
	 */
	float32e8_t calc_speed_holding_force(float32e8_t speed /* in m/s */, float32e8_t Frs /* in N */); /* in N */
protected:
	vehicle_summary_t vehicle;
	adverse_summary_t adverse;

	/**
	 * get force in kN according to current speed m/s in
	 */
	virtual sint32 get_force_summary(sint32 speed) = 0;
	virtual sint32 get_power_summary(sint32 speed) = 0;

	virtual const vehicle_summary_t &get_vehicle_summary() {
		return vehicle;
	}

	virtual const adverse_summary_t &get_adverse_summary() {
		return adverse;
	}

	virtual sint32 get_starting_force() { 
		return get_force_summary(0) * 1000; 
	}

	virtual sint32 get_continuous_power() { 
		return get_power_summary(get_vehicle_summary().max_speed) * 1000; 
	}
public:
	/**
	 * For calculating max speed at an arbitrary weight apply this result to your weight_summary_t() constructor as param sin_alpha.
	 */
	virtual sint16 get_current_friction() = 0;

	/** 
	 * Get maximum possible speed of convoy in km/h according to weight, power/force, inclination, etc.
	 * Depends on vehicle, adverse and given weight.
	 */
	sint32 calc_max_speed(const weight_summary_t &weight); 

	/** 
	 * Get maximum pullable weight at given inclination of convoy in kg according to maximum force, allowed maximum speed and continuous power.
	 * @param sin_alpha is 1000 times sin(inclination_angle). e.g. 50 == inclination of 2.8 per mille.
	 * Depends on vehicle and adverse.
	 */
	sint32 calc_max_weight(sint32 sin_alpha); 

	sint32 calc_max_starting_weight(sint32 sin_alpha);

	/** 
	 * Calculate the movement within delta_t
	 *
	 * @param akt_speed_soll is the desired end speed in simutrans speed.
	 * @param akt_speed is the current speed and returns the new speed after delta_t has gone in simutrans speed.
	 * @param sp_soll is the number of simutrans steps still to go and returns the new number of steps to go.
	 */
	void calc_move(long delta_t, const float32e8_t &simtime_factor, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll);
	virtual ~convoy_t(){}
};

/******************************************************************************/

enum convoy_detail_e
{
	cd_vehicle_summary = 0x01,
	cd_adverse_summary = 0x02,
	cd_freight_summary = 0x04,
	cd_weight_summary  = 0x08,
	cd_starting_force  = 0x10,
	cd_continuous_power = 0x20
};

class lazy_convoy_t /*abstract*/ : public convoy_t
{
private:
	freight_summary_t freight;
	sint32 starting_force;   // in N, calculated in get_starting_force()
	sint32 continuous_power; // in W, calculated in get_continuous_power()
protected:
	int is_valid;
	// decendents implement the update methods. 
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle) { (void)vehicle; } // = 0;
	virtual void update_adverse_summary(adverse_summary_t &adverse) { (void)adverse; } // = 0;
	virtual void update_freight_summary(freight_summary_t &freight) { (void)freight; } // = 0;
public:

	//-----------------------------------------------------------------------------
	
	// vehicle_summary becomes invalid, when the vehicle list or any vehicle's vehicle_besch_t changes.
	inline void invalidate_vehicle_summary()
	{
		is_valid &= ~(cd_vehicle_summary|cd_adverse_summary|cd_weight_summary|cd_starting_force|cd_continuous_power);
	}

	// vehicle_summary is valid if (is_valid & cd_vehicle_summary != 0)
	inline void validate_vehicle_summary() {
		if (!(is_valid & cd_vehicle_summary)) 
		{
			is_valid |= cd_vehicle_summary;
			update_vehicle_summary(vehicle);
		}
	}

	// vehicle_summary needs recaching only, if it is going to be used. 
	virtual const vehicle_summary_t &get_vehicle_summary() {
		validate_vehicle_summary();
		return convoy_t::get_vehicle_summary();
	}

	//-----------------------------------------------------------------------------
	
	// adverse_summary becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_besch_t or any vehicle's location/way changes.
	inline void invalidate_adverse_summary()
	{
		is_valid &= ~(cd_adverse_summary|cd_weight_summary);
	}

	// adverse_summary is valid if (is_valid & cd_adverse_summary != 0)
	inline void validate_adverse_summary() {
		if (!(is_valid & cd_adverse_summary)) 
		{
			is_valid |= cd_adverse_summary;
			update_adverse_summary(adverse);
		}
	}

	// adverse_summary needs recaching only, if it is going to be used. 
	virtual const adverse_summary_t &get_adverse_summary() {
		validate_adverse_summary();
		return convoy_t::get_adverse_summary();
	}

	//-----------------------------------------------------------------------------
	
	// freight_summary becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_besch_t.
	inline void invalidate_freight_summary()
	{
		is_valid &= ~(cd_freight_summary);
	}

	// freight_summary is valid if (is_valid & cd_freight_summary != 0)
	inline void validate_freight_summary() {
		if (!(is_valid & cd_freight_summary)) 
		{
			is_valid |= cd_freight_summary;
			update_freight_summary(freight);
		}
	}

	// freight_summary needs recaching only, if it is going to be used. 
	virtual const freight_summary_t &get_freight_summary() {
		validate_freight_summary();
		return freight;
	}

	//-----------------------------------------------------------------------------
	
	// starting_force becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_besch_t.
	inline void invalidate_starting_force()
	{
		is_valid &= ~(cd_starting_force);
	}

	virtual sint32 get_starting_force() 
	{
		if (!(is_valid & cd_starting_force)) 
		{
			is_valid |= cd_starting_force;
			starting_force = convoy_t::get_starting_force();
		}
		return starting_force;
	}

	//-----------------------------------------------------------------------------
	
	// continuous_power becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_besch_t.
	inline void invalidate_continuous_power()
	{
		is_valid &= ~(cd_continuous_power);
	}

	virtual sint32 get_continuous_power()
	{
		if (!(is_valid & cd_continuous_power)) 
		{
			is_valid |= cd_continuous_power;
			continuous_power = convoy_t::get_continuous_power();
		}
		return continuous_power;
	}

	//-----------------------------------------------------------------------------

	lazy_convoy_t() : convoy_t()
	{
		is_valid = 0;
	}

	sint32 calc_max_speed(const weight_summary_t &weight)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_speed(weight);
	}

	sint32 calc_max_weight(sint32 sin_alpha)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_weight(sin_alpha);
	}

	sint32 calc_max_starting_weight(sint32 sin_alpha)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_starting_weight(sin_alpha);
	}

	void calc_move(long delta_t, const float32e8_t &simtime_factor, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		convoy_t::calc_move(delta_t, simtime_factor, weight, akt_speed_soll, akt_speed, sp_soll);
	}
	virtual ~lazy_convoy_t(){}
};

/******************************************************************************/

class potential_convoy_t : public lazy_convoy_t
{
private:
	vector_tpl<const vehikel_besch_t *> &vehicles;
	karte_t &world;
protected:
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle);
	virtual void update_adverse_summary(adverse_summary_t &adverse);
	virtual void update_freight_summary(freight_summary_t &freight);
	virtual sint32 get_force_summary(sint32 speed /* in m/s */);
	virtual sint32 get_power_summary(sint32 speed /* in m/s */);
public:
	potential_convoy_t(karte_t &world, vector_tpl<const vehikel_besch_t *> &besch) : lazy_convoy_t(), vehicles(besch), world(world)
	{
	}
	virtual sint16 get_current_friction();
	virtual ~potential_convoy_t(){}
};

/******************************************************************************/

class vehicle_as_potential_convoy_t : public potential_convoy_t
{
private:
	vector_tpl<const vehikel_besch_t *> vehicles;
public:
	vehicle_as_potential_convoy_t(karte_t &world, const vehikel_besch_t &besch) : potential_convoy_t(world, vehicles)
	{
		vehicles.append(&besch);
	}
};

/******************************************************************************/

class existing_convoy_t : public lazy_convoy_t
{
private:
	convoi_t &convoy;
	weight_summary_t weight;
protected:
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle);
	virtual void update_adverse_summary(adverse_summary_t &adverse);
	virtual void update_freight_summary(freight_summary_t &freight);
	virtual void update_weight_summary(weight_summary_t &weight);
	virtual sint32 get_force_summary(sint32 speed /* in m/s */);
	virtual sint32 get_power_summary(sint32 speed /* in m/s */);
public:
	existing_convoy_t(convoi_t &vehicles) : lazy_convoy_t(), convoy(vehicles)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
	}
	virtual ~existing_convoy_t(){}

	//-----------------------------------------------------------------------------

	virtual sint16 get_current_friction();
	
	// weight_summary becomes invalid, when vehicle_summary or envirion_summary 
	// becomes invalid.
	inline void invalidate_weight_summary()
	{
		is_valid &= ~cd_weight_summary;
	}

	// weight_summary is valid if (is_valid & cd_weight_summary != 0)
	inline void validate_weight_summary() {
		if (!(is_valid & cd_weight_summary)) 
		{
			is_valid |= cd_weight_summary;
			update_weight_summary(weight);
		}
	}

	// weight_summary needs recaching only, if it is going to be used. 
	inline const weight_summary_t &get_weight_summary() {
		validate_weight_summary();
		return weight;
	}

	inline void calc_move(long delta_t, const float32e8_t &simtime_factor, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
	{
		validate_weight_summary();
		convoy_t::calc_move(delta_t, simtime_factor, weight, akt_speed_soll, akt_speed, sp_soll);
	}

};

#endif
