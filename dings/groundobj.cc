/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../player/simplay.h"
#include "../simtools.h"
#include "../simtypes.h"

#include "../boden/grund.h"

#include "../besch/groundobj_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/freelist.h"


#include "groundobj.h"

/******************************** static stuff for forest rules ****************************************************************/


/*
 * Diese Tabelle erm�glicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const groundobj_besch_t *> groundobj_t::groundobj_typen(0);

/*
 * Diese Tabelle erm�glicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<groundobj_besch_t *> groundobj_t::besch_names;


bool compare_groundobj_besch(const groundobj_besch_t* a, const groundobj_besch_t* b)
{
	return strcmp(a->get_name(), b->get_name())<0;
}

bool groundobj_t::alles_geladen()
{
	groundobj_typen.resize(besch_names.get_count());
	stringhashtable_iterator_tpl<groundobj_besch_t *>iter(besch_names);
	while(  iter.next()  ) {
		groundobj_typen.insert_ordered( iter.get_current_value(), compare_groundobj_besch );
	}
	// iterate again to assign the index
	stringhashtable_iterator_tpl<groundobj_besch_t *>iter2(besch_names);
	while(  iter2.next()  ) {
		iter2.access_current_value()->index = groundobj_typen.index_of( iter2.get_current_value());
	}

	if(besch_names.empty()) {
		groundobj_typen.append( NULL );
		DBG_MESSAGE("groundobj_t", "No groundobj found - feature disabled");
	}
	return true;
}



bool groundobj_t::register_besch(groundobj_besch_t *besch)
{
	assert(besch->get_speed()==0);
	// remove duplicates
	if(  besch_names.remove( besch->get_name() )  ) {
		dbg->warning( "groundobj_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	besch_names.put(besch->get_name(), besch );
	return true;
}




/* also checks for distribution values
 * @author prissi
 */
const groundobj_besch_t *groundobj_t::random_groundobj_for_climate(climate cl, hang_t::typ slope  )
{
	// none there
	if(  besch_names.empty()  ) {
		return NULL;
	}

	int weight = 0;
	for(  unsigned i=0;  i<groundobj_typen.get_count();  i++  ) {
		if(groundobj_typen[i] == NULL)
		{
			break;
		}
		if(  groundobj_typen[i]->is_allowed_climate(cl)  &&  (slope==hang_t::flach  ||  groundobj_typen[i]->get_phases()==16)  ) {
			weight += groundobj_typen[i]->get_distribution_weight();
		}
	}

	// now weight their distribution
	if(  weight > 0  ) {
		const int w=simrand(weight, "const groundobj_besch_t *groundobj_t::random_groundobj_for_climate(");
		weight = 0;
		for(  unsigned i=0;  i<groundobj_typen.get_count();  i++  ) {
			if(  groundobj_typen[i]->is_allowed_climate(cl)  &&  (slope==hang_t::flach  ||  groundobj_typen[i]->get_phases()==16)  ) {
				weight += groundobj_typen[i]->get_distribution_weight();
				if(weight>=w) {
					return groundobj_typen[i];
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void groundobj_t::calc_bild()
{
	// alter/2048 is the age of the tree
	const groundobj_besch_t *besch=get_besch();
	const sint16 seasons = besch->get_seasons()-1;
	season=0;

	// two possibilities
	switch(seasons) {
				// summer only
		case 0: season = 0;
				break;
				// summer, snow
		case 1: season = welt->get_snowline()<=get_pos().z;
				break;
				// summer, winter, snow
		case 2: season = welt->get_snowline()<=get_pos().z ? 2 : welt->get_jahreszeit()==1;
				break;
		default: if(welt->get_snowline()<=get_pos().z) {
					season = seasons;
				}
				else {
					// welt->get_yearsteps(): resolution 1/8th month (0..95)
					season = (seasons * (welt->get_yearsteps() + 1) - 1) / 96;

				}
				break;
	}
	// check for slopes?
	uint16 phase = 0;
	if(besch->get_phases()==16) {
		phase = welt->lookup(get_pos())->get_grund_hang();
	}
	const bild_besch_t *bild_ptr = get_besch()->get_bild( season, phase );
	bild = bild_ptr ? bild_ptr->get_nummer() : IMG_LEER;
}



groundobj_t::groundobj_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	rdwr(file);
}



groundobj_t::groundobj_t(karte_t *welt, koord3d pos, const groundobj_besch_t *b ) : ding_t(welt, pos)
{
	season = 0xF;	// mark dirty
	groundobjtype = groundobj_typen.index_of(b);
	calc_bild();
}



bool groundobj_t::check_season(long )
{
	const uint8 old_season = season;
	calc_bild();
	if(season!=old_season) {
		mark_image_dirty( get_bild(), 0 );
	}
	return true;
}



void groundobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "groundobj_t" );

	ding_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		groundobj_besch_t *besch = besch_names.get(bname);
		if(  besch_names.empty()  ||  besch==NULL  ) {
			groundobjtype = simrand(groundobj_typen.get_count(), "void groundobj_t::rdwr");
		}
		else {
			groundobjtype = besch->get_index();
		}
		// if not there, besch will be zero
	}
}



/**
 * �ffnet ein neues Beobachtungsfenster f�r das Objekt.
 * @author Hj. Malthaner
 */
void groundobj_t::zeige_info()
{
	if(umgebung_t::tree_info) {
		ding_t::zeige_info();
	}
}



/**
 * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void groundobj_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	buf.append(translator::translate(get_besch()->get_name()));
	if (char const* const maker = get_besch()->get_copyright()) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, get_besch()->get_preis()/100.0 );
	buf.append( buffer );
}



void groundobj_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, -get_besch()->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( get_bild(), 0 );
}



void *groundobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(groundobj_t));
}



void groundobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(groundobj_t),p);
}
