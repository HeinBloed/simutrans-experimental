/*
 * runways f�r Simutrans
 *
 * �berarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"
#include "../../dataobj/loadsave.h"

#include "runway.h"

const weg_besch_t *runway_t::default_runway=NULL;


runway_t::runway_t(karte_t *welt) : schiene_t(welt)
{
	set_besch(default_runway);
}


runway_t::runway_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}


void runway_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "runway_t" );

	weg_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const weg_besch_t *besch = wegbauer_t::get_besch(bname);
		int old_max_speed=get_max_speed();
		if(besch==NULL) {
			besch = wegbauer_t::weg_search(air_wt,old_max_speed>0 ? old_max_speed : 20, 0, (weg_t::system_type)(old_max_speed>250) );
			if(besch==NULL) {
				besch = default_runway;
			}
			dbg->warning("runway_t::rdwr()", "Unknown runway %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
		}
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		set_besch(besch);
	}
}
