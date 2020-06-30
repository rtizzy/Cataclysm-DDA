#include "computer.h"

#include <algorithm>
#include <cstdlib>
#include <locale>
#include <sstream>

#include "debug.h"
#include "enum_conversions.h"
#include "json.h"
#include "output.h"
#include "translations.h"

template <typename E> struct enum_traits;

computer_option::computer_option()
    : name( "Unknown" ), action( COMPACT_NULL ), security( 0 )
{
}

computer_option::computer_option( const std::string &N, computer_action A, int S )
    : name( N ), action( A ), security( S )
{
}

void computer_option::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "name", name );
    jout.member( "action" );
    jout.write_as_string( action );
    jout.member( "security", security );
    jout.end_object();
}

void computer_option::deserialize( JsonIn &jin )
{
    const JsonObject jo = jin.get_object();
    name = jo.get_string( "name" );
    action = jo.get_enum_value<computer_action>( "action" );
    security = jo.get_int( "security" );
}

computer_failure::computer_failure()
    : type( COMPFAIL_NULL )
{
}

void computer_failure::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "action" );
    jout.write_as_string( type );
    jout.end_object();
}

void computer_failure::deserialize( JsonIn &jin )
{
    const JsonObject jo = jin.get_object();
    type = jo.get_enum_value<computer_failure_type>( "action" );
}

computer::computer( const std::string &new_name, int new_security )
    : name( new_name ), mission_id( -1 ), security( new_security ), alerts( 0 ),
      next_attempt( calendar::before_time_starts ),
      access_denied( _( "ERROR!  Access denied!" ) )
{
}

void computer::set_security( int Security )
{
    security = Security;
}

void computer::add_option( const computer_option &opt )
{
    options.emplace_back( opt );
}

void computer::add_option( const std::string &opt_name, computer_action action,
                           int security )
{
    add_option( computer_option( opt_name, action, security ) );
}

void computer::add_failure( const computer_failure &failure )
{
    failures.emplace_back( failure );
}

void computer::add_failure( computer_failure_type failure )
{
    add_failure( computer_failure( failure ) );
}

void computer::set_access_denied_msg( const std::string &new_msg )
{
    access_denied = new_msg;
}

void computer::set_mission( const int id )
{
    mission_id = id;
}

static computer_action computer_action_from_legacy_enum( int val );
static computer_failure_type computer_failure_type_from_legacy_enum( int val );

void computer::load_legacy_data( const std::string &data )
{
    options.clear();
    failures.clear();

    std::istringstream dump( data );
    dump.imbue( std::locale::classic() );

    dump >> name >> security >> mission_id;

    name = string_replace( name, "_", " " );

    // Pull in options
    int optsize;
    dump >> optsize;
    for( int n = 0; n < optsize; n++ ) {
        std::string tmpname;

        int tmpaction;
        int tmpsec;

        dump >> tmpname >> tmpaction >> tmpsec;
        // Legacy missle launch option that got removed before `computer_action` was
        // refactored to be saved and loaded as string ids. Do not change this number:
        // `computer_action` now has different underlying values from back then!
        if( tmpaction == 15 ) {
            continue;
        }
        add_option( string_replace( tmpname, "_", " " ), computer_action_from_legacy_enum( tmpaction ),
                    tmpsec );
    }

    // Pull in failures
    int failsize;
    dump >> failsize;
    for( int n = 0; n < failsize; n++ ) {
        int tmpfail;
        dump >> tmpfail;
        add_failure( computer_failure_type_from_legacy_enum( tmpfail ) );
    }

    std::string tmp_access_denied;
    dump >> tmp_access_denied;

    // For backwards compatibility, only set the access denied message if it
    // isn't empty. This is to avoid the message becoming blank when people
    // load old saves.
    if( !tmp_access_denied.empty() ) {
        access_denied = string_replace( tmp_access_denied, "_", " " );
    }
}

<<<<<<< HEAD
void computer::activate_function( computer_action action )
{
    // Token move cost for any action, if an action takes longer decrement moves further.
    g->u.moves -= 30;
    switch( action ) {

        case COMPACT_NULL: // Unknown action.
        case NUM_COMPUTER_ACTIONS: // Suppress compiler warning [-Wswitch]
            break;

        // OPEN_DISARM falls through to just OPEN
        case COMPACT_OPEN_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_OPEN:
            g->m.translate_radius( t_door_metal_locked, t_floor, 25.0, g->u.pos(), true );
            query_any( _( "Doors opened.  Press any key..." ) );
            break;

        //LOCK AND UNLOCK are used to build more complex buildings
        // that can have multiple doors that can be locked and be
        // unlocked by different computers.
        //Simply uses translate_radius which take a given radius and
        // player position to determine which terrain tiles to edit.
        case COMPACT_LOCK:
            g->m.translate_radius( t_door_metal_c, t_door_metal_locked, 8.0, g->u.pos(), true );
            query_any( _( "Lock enabled.  Press any key..." ) );
            break;

        // UNLOCK_DISARM falls through to just UNLOCK
        case COMPACT_UNLOCK_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_UNLOCK:
            g->m.translate_radius( t_door_metal_locked, t_door_metal_c, 8.0, g->u.pos(), true );
            query_any( _( "Lock disabled.  Press any key..." ) );
            break;

        //Toll is required for the church computer/mechanism to function
        case COMPACT_TOLL:
            //~ the sound of a church bell ringing
            sounds::sound( g->u.pos(), 120, sounds::sound_t::music,
                           _( "Bohm... Bohm... Bohm..." ), true, "environment", "church_bells" );
            break;

        case COMPACT_SAMPLE:
            g->u.moves -= 30;
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        for( int x1 = x - 1; x1 <= x + 1; x1++ ) {
                            for( int y1 = y - 1; y1 <= y + 1; y1++ ) {
                                if( g->m.furn( x1, y1 ) == f_counter ) {
                                    bool found_item = false;
                                    item sewage( "sewage", calendar::turn );
                                    auto candidates = g->m.i_at( x1, y1 );
                                    for( auto &candidate : candidates ) {
                                        int capa = candidate.get_remaining_capacity_for_liquid( sewage );
                                        if( capa <= 0 ) {
                                            continue;
                                        }
                                        item &elem = candidate;
                                        capa = std::min( sewage.charges, capa );
                                        if( elem.contents.empty() ) {
                                            elem.put_in( sewage );
                                            elem.contents.front().charges = capa;
                                        } else {
                                            elem.contents.front().charges += capa;
                                        }
                                        found_item = true;
                                        break;
                                    }
                                    if( !found_item ) {
                                        g->m.add_item_or_charges( x1, y1, sewage );
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;

        case COMPACT_RELEASE:
            g->u.add_memorial_log( pgettext( "memorial_male", "Released subspace specimens." ),
                                   pgettext( "memorial_female", "Released subspace specimens." ) );
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 25.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key..." ) );
            break;

        // COMPACT_RELEASE_DISARM falls through to just COMPACT_RELEASE_BIONICS
        case COMPACT_RELEASE_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_RELEASE_BIONICS:
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 3.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key..." ) );
            break;

        case COMPACT_TERMINATE:
            g->u.add_memorial_log( pgettext( "memorial_male", "Terminated subspace specimens." ),
                                   pgettext( "memorial_female", "Terminated subspace specimens." ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    tripoint p( x, y, g->u.posz() );
                    monster *const mon = g->critter_at<monster>( p );
                    if( mon &&
                        ( ( g->m.ter( x, y - 1 ) == t_reinforced_glass &&
                            g->m.ter( x, y + 1 ) == t_concrete_wall ) ||
                          ( g->m.ter( x, y + 1 ) == t_reinforced_glass &&
                            g->m.ter( x, y - 1 ) == t_concrete_wall ) ) ) {
                        mon->die( &g->u );
                    }
                }
            }
            query_any( _( "Subjects terminated.  Press any key..." ) );
            break;

        case COMPACT_PORTAL: {
            g->u.add_memorial_log( pgettext( "memorial_male", "Opened a portal." ),
                                   pgettext( "memorial_female", "Opened a portal." ) );
            tripoint tmp = g->u.pos();
            int &i = tmp.x;
            int &j = tmp.y;
            for( i = 0; i < MAPSIZE_X; i++ ) {
                for( j = 0; j < MAPSIZE_Y; j++ ) {
                    int numtowers = 0;
                    tripoint tmp2 = tmp;
                    int &xt = tmp2.x;
                    int &yt = tmp2.y;
                    for( xt = i - 2; xt <= i + 2; xt++ ) {
                        for( yt = j - 2; yt <= j + 2; yt++ ) {
                            if( g->m.ter( tmp2 ) == t_radio_tower ) {
                                numtowers++;
                            }
                        }
                    }
                    if( numtowers >= 4 ) {
                        if( g->m.tr_at( tmp ).id == trap_str_id( "tr_portal" ) ) {
                            g->m.remove_trap( tmp );
                        } else {
                            g->m.trap_set( tmp, tr_portal );
                        }
                    }
                }
            }
        }
        break;

        case COMPACT_CASCADE: {
            if( !query_bool( _( "WARNING: Resonance cascade carries severe risk!  Continue?" ) ) ) {
                return;
            }
            g->u.add_memorial_log( pgettext( "memorial_male", "Caused a resonance cascade." ),
                                   pgettext( "memorial_female", "Caused a resonance cascade." ) );
            std::vector<tripoint> cascade_points;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.ter( dest ) == t_radio_tower ) {
                    cascade_points.push_back( dest );
                }
            }
            explosion_handler::resonance_cascade( random_entry( cascade_points, g->u.pos() ) );
        }
        break;

        case COMPACT_RESEARCH: {
            // TODO: seed should probably be a member of the computer, or better: of the computer action.
            // It is here to ensure one computer reporting the same text on each invocation.
            const int seed = g->get_levx() + g->get_levy() + g->get_levz() + alerts;
            std::string log = SNIPPET.get( SNIPPET.assign( "lab_notes", seed ) );
            if( log.empty() ) {
                log = _( "No data found." );
            } else {
                g->u.moves -= 70;
            }

            print_text( "%s", log );
            // One's an anomaly
            if( alerts == 0 ) {
                query_any( _( "Local data-access error logged, alerting helpdesk. Press any key..." ) );
                alerts ++;
            } else {
                // Two's a trend.
                query_any(
                    _( "Warning: anomalous archive-access activity detected at this node. Press any key..." ) );
                alerts ++;
            }
        }
        break;

        case COMPACT_RADIO_ARCHIVE: {
            g->u.moves -= 300;
            sfx::play_ambient_variant_sound( "radio", "inaudible_chatter", 100, 21, 2000 );
            print_text( "Accessing archive. Playing audio recording nr %d.\n%s", rng( 1, 9999 ),
                        SNIPPET.random_from_category( "radio_archive" ) );
            if( one_in( 3 ) ) {
                query_any( _( "Warning: resticted data access. Attempt logged. Press any key..." ) );
                alerts ++;
            } else {
                query_any( _( "Press any key..." ) );
            }
            sfx::fade_audio_channel( 21, 100 );
        }
        break;

        case COMPACT_MAPS: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            overmap_buffer.reveal( point( center.x, center.y ), 40, 0 );
            query_any(
                _( "Surface map data downloaded.  Local anomalous-access error logged.  Press any key..." ) );
            remove_option( COMPACT_MAPS );
            alerts ++;
        }
        break;

        case COMPACT_MAP_SEWER: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            for( int i = -60; i <= 60; i++ ) {
                for( int j = -60; j <= 60; j++ ) {
                    const oter_id &oter = overmap_buffer.ter( center.x + i, center.y + j, center.z );
                    if( is_ot_match( "sewer", oter, ot_match_type::type ) ||
                        is_ot_match( "sewage", oter, ot_match_type::prefix ) ) {
                        overmap_buffer.set_seen( center.x + i, center.y + j, center.z, true );
                    }
                }
            }
            query_any( _( "Sewage map data downloaded.  Press any key..." ) );
            remove_option( COMPACT_MAP_SEWER );
        }
        break;

        case COMPACT_MAP_SUBWAY: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            for( int i = -60; i <= 60; i++ ) {
                for( int j = -60; j <= 60; j++ ) {
                    const oter_id &oter = overmap_buffer.ter( center.x + i, center.y + j, center.z );
                    if( is_ot_match( "subway", oter, ot_match_type::type ) ||
                        is_ot_match( "lab_train_depot", oter, ot_match_type::contains ) ) {
                        overmap_buffer.set_seen( center.x + i, center.y + j, center.z, true );
                    }
                }
            }
            query_any( _( "Subway map data downloaded.  Press any key..." ) );
            remove_option( COMPACT_MAP_SUBWAY );
        }
        break;

        case COMPACT_MISS_LAUNCH: {
            // Target Acquisition.
            tripoint target = ui::omap::choose_point( 0 );
            if( target == overmap::invalid_tripoint ) {
                add_msg( m_info, _( "Target acquisition canceled." ) );
                return;
            }
            if( query_yn( _( "Confirm nuclear missile launch." ) ) ) {
                add_msg( m_info, _( "Nuclear missile launched!" ) );
                //Remove the option to fire another missile.
                options.clear();
            } else {
                add_msg( m_info, _( "Nuclear missile launch aborted." ) );
                return;
            }
            g->refresh_all();

            //Put some smoke gas and explosions at the nuke location.
            for( int i = g->u.posx() + 8; i < g->u.posx() + 15; i++ ) {
                for( int j = g->u.posy() + 3; j < g->u.posy() + 12; j++ ) {
                    if( !one_in( 4 ) ) {
                        tripoint dest( i + rng( -2, 2 ), j + rng( -2, 2 ), g->u.posz() );
                        g->m.add_field( dest, fd_smoke, rng( 1, 9 ) );
                    }
                }
            }

            explosion_handler::explosion( tripoint( g->u.posx() + 10, g->u.posx() + 21, g->get_levz() ), 200,
                                          0.7,
                                          true ); //Only explode once. But make it large.

            //...ERASE MISSILE, OPEN SILO, DISABLE COMPUTER
            // For each level between here and the surface, remove the missile
            for( int level = g->get_levz(); level <= 0; level++ ) {
                map tmpmap;
                tmpmap.load( g->get_levx(), g->get_levy(), level, false );

                if( level < 0 ) {
                    tmpmap.translate( t_missile, t_hole );
                } else {
                    tmpmap.translate( t_metal_floor, t_hole );
                }
                tmpmap.save();
            }

            const oter_id oter = overmap_buffer.ter( target.x, target.y, 0 );
            //~ %s is terrain name
            g->u.add_memorial_log( pgettext( "memorial_male", "Launched a nuke at a %s." ),
                                   pgettext( "memorial_female", "Launched a nuke at a %s." ),
                                   oter->get_name() );
            for( int x = target.x - 2; x <= target.x + 2; x++ ) {
                for( int y = target.y - 2; y <= target.y + 2; y++ ) {
                    // give it a nice rounded shape
                    if( !( x == ( target.x - 2 ) && ( y == ( target.y - 2 ) ) ) &&
                        !( x == ( target.x - 2 ) && ( y == ( target.y + 2 ) ) ) &&
                        !( x == ( target.x + 2 ) && ( y == ( target.y - 2 ) ) ) &&
                        !( x == ( target.x + 2 ) && ( y == ( target.y + 2 ) ) ) ) {
                        // TODO: Z
                        explosion_handler::nuke( tripoint( x, y, 0 ) );
                    }

                }
            }

            activate_failure( COMPFAIL_SHUTDOWN );
        }
        break;

        case COMPACT_MISS_DISARM: // TODO: stop the nuke from creating radioactive clouds.
            if( query_yn( _( "Disarm missile." ) ) ) {
                g->u.add_memorial_log( pgettext( "memorial_male", "Disarmed a nuclear missile." ),
                                       pgettext( "memorial_female", "Disarmed a nuclear missile." ) );
                add_msg( m_info, _( "Nuclear missile disarmed!" ) );
                //disable missile.
                options.clear();
                activate_failure( COMPFAIL_SHUTDOWN );
            } else {
                add_msg( m_neutral, _( "Nuclear missile remains active." ) );
                return;
            }
            break;

        case COMPACT_LIST_BIONICS: {
            g->u.moves -= 30;
            std::vector<std::string> names;
            int more = 0;
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    for( auto &elem : g->m.i_at( x, y ) ) {
                        if( elem.is_bionic() ) {
                            if( static_cast<int>( names.size() ) < TERMY - 8 ) {
                                names.push_back( elem.tname() );
                            } else {
                                more++;
                            }
                        }
                    }
                }
            }

            reset_terminal();

            print_newline();
            print_line( _( "Bionic access - Manifest:" ) );
            print_newline();

            for( auto &name : names ) {
                print_line( "%s", name );
            }
            if( more > 0 ) {
                print_line( ngettext( "%d OTHER FOUND...", "%d OTHERS FOUND...", more ), more );
            }

            print_newline();
            query_any( _( "Press any key..." ) );
        }
        break;

        case COMPACT_ELEVATOR_ON:
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_elevator_control_off ) {
                        g->m.ter_set( x, y, t_elevator_control );
                    }
                }
            }
            query_any( _( "Elevator activated.  Press any key..." ) );
            break;

        case COMPACT_AMIGARA_LOG: {
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_text( "%s", SNIPPET.get( SNIPPET.assign( "amigara1" ) ) );

            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_text( "%s", SNIPPET.get( SNIPPET.assign( "amigara2" ) ) );

            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_text( "%s", SNIPPET.get( SNIPPET.assign( "amigara3" ) ) );

            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            reset_terminal();
            for( int i = 0; i < 10; i++ ) {
                print_gibberish_line();
            }
            print_newline();
            print_newline();
            print_newline();
            print_line( _( "AMIGARA PROJECT" ) );
            print_newline();
            print_newline();
            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "\
SITE %d%d%d\n\
PERTINENT FOREMAN LOGS WILL BE PREPENDED TO NOTES" ),
                        g->get_levx(), g->get_levy(), abs( g->get_levz() ) );
            print_text( "%s", SNIPPET.get( SNIPPET.assign( "amigara4" ) ) );
            print_gibberish_line();
            print_gibberish_line();
            print_newline();
            print_error( _( "FILE CORRUPTED, PRESS ANY KEY..." ) );
            inp_mngr.wait_for_any_key();
            reset_terminal();
            break;
        }

        case COMPACT_AMIGARA_START:
            g->events.add( EVENT_AMIGARA, calendar::turn + 1_minutes );
            if( !g->u.has_artifact_with( AEP_PSYSHIELD ) ) {
                g->u.add_effect( effect_amigara, 2_minutes );
            }
            // Disable this action to prevent further amigara events, which would lead to
            // further amigara monster, which would lead to further artifacts.
            remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER:
            for( auto miss : g->u.get_active_missions() ) {
                static const mission_type_id commo_2 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_2" );
                if( miss->mission_id() == commo_2 ) {
                    print_error( _( "--ACCESS GRANTED--" ) );
                    print_error( _( "Mission Complete!" ) );
                    miss->step_complete( 1 );
                    inp_mngr.wait_for_any_key();
                    return;
                    //break;
                }
            }
            print_error( _( "ACCESS DENIED" ) );
            inp_mngr.wait_for_any_key();
            break;

        case COMPACT_REPEATER_MOD:
            if( g->u.has_amount( "radio_repeater_mod", 1 ) ) {
                for( auto miss : g->u.get_active_missions() ) {
                    static const mission_type_id commo_3 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_3" ),
                                                 commo_4 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_4" );
                    if( miss->mission_id() == commo_3 || miss->mission_id() == commo_4 ) {
                        miss->step_complete( 1 );
                        print_error( _( "Repeater mod installed..." ) );
                        print_error( _( "Mission Complete!" ) );
                        g->u.use_amount( "radio_repeater_mod", 1 );
                        inp_mngr.wait_for_any_key();
                        options.clear();
                        activate_failure( COMPFAIL_SHUTDOWN );
                        break;
                    }
                }
            } else {
                print_error( _( "You do not have a repeater mod to install..." ) );
                inp_mngr.wait_for_any_key();
                break;
            }
            break;

        case COMPACT_DOWNLOAD_SOFTWARE:
            if( item *const usb = pick_usb() ) {
                mission *miss = mission::find( mission_id );
                if( miss == nullptr ) {
                    debugmsg( _( "Computer couldn't find its mission!" ) );
                    return;
                }
                g->u.moves -= 30;
                item software( miss->get_item_id(), 0 );
                software.mission_id = mission_id;
                usb->contents.clear();
                usb->put_in( software );
                print_line( _( "Software downloaded." ) );
            } else {
                print_error( _( "USB drive required!" ) );
            }
            inp_mngr.wait_for_any_key();
            break;

        case COMPACT_BLOOD_ANAL:
            g->u.moves -= 70;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_centrifuge ) {
                    map_stack items = g->m.i_at( dest );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place sample in centrifuge." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
                    } else if( items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Please only use container with blood sample." ) );
                    } else if( items.only_item().contents.front().typeId() != "blood" ) {
                        print_error( _( "ERROR: Please only use blood samples." ) );
                    } else { // Success!
                        const item &blood = items.only_item().contents.front();
                        const mtype *mt = blood.get_mtype();
                        if( mt == nullptr || mt->id == mtype_id::NULL_ID() ) {
                            print_line( _( "Result:  Human blood, no pathogens found." ) );
                        } else if( mt->in_species( ZOMBIE ) ) {
                            if( mt->in_species( HUMAN ) ) {
                                print_line( _( "Result:  Human blood.  Unknown pathogen found." ) );
                            } else {
                                print_line( _( "Result:  Unknown blood type.  Unknown pathogen found." ) );
                            }
                            print_line( _( "Pathogen bonded to erythrocytes and leukocytes." ) );
                            if( query_bool( _( "Download data?" ) ) ) {
                                if( item *const usb = pick_usb() ) {
                                    item software( "software_blood_data", 0 );
                                    usb->contents.clear();
                                    usb->put_in( software );
                                    print_line( _( "Software downloaded." ) );
                                } else {
                                    print_error( _( "USB drive required!" ) );
                                }
                            }
                        } else {
                            print_line( _( "Result: Unknown blood type.  Test non-conclusive." ) );
                        }
                    }
                }
            }
            query_any( _( "Press any key..." ) );
            break;

        case COMPACT_DATA_ANAL:
            g->u.moves -= 30;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_floor_blue ) {
                    print_error( _( "PROCESSING DATA" ) );
                    map_stack items = g->m.i_at( dest );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place memory bank in scan area." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please only scan one item at a time." ) );
                    } else if( items.only_item().typeId() != "usb_drive" &&
                               items.only_item().typeId() != "black_box" ) {
                        print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                    } else if( items.only_item().typeId() == "usb_drive" && items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Memory bank is empty." ) );
                    } else { // Success!
                        if( items.only_item().typeId() == "black_box" ) {
                            print_line( _( "Memory Bank:  Military Hexron Encryption\nPrinting Transcript\n" ) );
                            item transcript( "black_box_transcript", calendar::turn );
                            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), transcript );
                        } else {
                            print_line( _( "Memory Bank:  Unencrypted\nNothing of interest.\n" ) );
                        }
                    }
                }
            }
            query_any( _( "Press any key..." ) );
            break;

        case COMPACT_DISCONNECT:
            reset_terminal();
            print_line( _( "\n\
ERROR:  NETWORK DISCONNECT \n\
UNABLE TO REACH NETWORK ROUTER OR PROXY.  PLEASE CONTACT YOUR\n\
SYSTEM ADMINISTRATOR TO RESOLVE THIS ISSUE.\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_EMERG_MESS:
            print_line( _( "\
GREETINGS CITIZEN. A BIOLOGICAL ATTACK HAS TAKEN PLACE AND A STATE OF \n\
EMERGENCY HAS BEEN DECLARED. EMERGENCY PERSONNEL WILL BE AIDING YOU \n\
SHORTLY. TO ENSURE YOUR SAFETY PLEASE FOLLOW THE STEPS BELOW. \n\
\n\
1. PLEASE REMAIN CALM \n\
2. PLEASE REMAIN INSIDE THE BUILDING. \n\
3. PLEASE SEEK SHELTER IN THE BASEMENT. \n\
4. IN CASE OF CHEMICAL ATTACK, UTILIZE PROVIDED GAS MASKS. \n\
5. PLEASE AWAIT FURTHER INSTRUCTIONS FROM YOUR GOVERNMENT REPRESENTATIVES. \n\
\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_EMERG_REF_CENTER:
            reset_terminal();
            mark_refugee_center();
            reset_terminal();
            break;

        case COMPACT_TOWER_UNRESPONSIVE:
            print_line( _( "\
  WARNING, RADIO TOWER IS UNRESPONSIVE. \n\
  \n\
  BACKUP POWER INSUFFICIENT TO MEET BROADCASTING REQUIREMENTS. \n\
  IN THE EVENT OF AN EMERGENCY, CONTACT LOCAL NATIONAL GUARD \n\
  UNITS TO RECEIVE PRIORITY WHEN GENERATORS ARE BEING DEPLOYED. \n\
  \n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR1_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
      I want to remind everyone on staff: Do not open or examine\n\
  containers above your security-clearance.  If you have some\n\
  question about safety protocols or shipping procedures, please\n\
  contact your SRCF administrator or on-site military officer.\n\
  When in doubt, assume all containers are Class-A Biohazards\n\
  and highly toxic. Take full precautions!\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR2_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  From today onward medical wastes are not to be stored anywhere\n\
  near radioactive materials.  All containers are to be\n\
  re-arranged according to these new regulations.  If your\n\
  facility currently has these containers stored in close\n\
  proximity, you are to work with armed guards on duty at all\n\
  times. Report any unusual activity to your SRCF administrator\n\
  at once.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR3_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  Worker health and safety is our number one concern!  As such,\n\
  we are instituting weekly health examinations for all SRCF\n\
  employees.  Report any unusual symptoms or physical changes\n\
  to your SRCF administrator at once.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR4_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From:  Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  All compromised facilities will remain under lock down until\n\
  further notice.  Anyone who has seen or come in direct contact\n\
  with the creatures is to report to the home office for a full\n\
  medical evaluation and security debriefing.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_1_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: EPA: Report All Potential Containment Breaches 3873643\n\
  To: all SRCF staff\n\
  From:  Robert Shane, Director of the EPA\n\
  \n\
  All hazardous waste dumps and sarcophagi must submit three\n\
  samples from each operational leache system to the following\n\
  addresses:\n\
  \n\
  CDC Bioterrorism Lab \n\
  Building 10\n\
  Corporate Square Boulevard\n\
  Atlanta, GA 30329\n\
  \n\
  EPA Region 8 Laboratory\n\
  16194 W. 45th Drive\n\
  Golden, Colorado 80403\n\
  \n\
  These samples must be accurate and any attempts to cover\n\
  incompetencies will result in charges of Federal Corruption\n\
  and potentially Treason.\n" ) );
            query_any( _( "Press any key to continue..." ) );
            reset_terminal();
            print_line( _( "Director of the EPA,\n\
  Robert Shane\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_2_MESS:
            reset_terminal();
            print_line( _( " Subj: SRCF: Internal Memo, EPA [2918024]\n\
  To: all SRCF admin staff\n\
  From:  Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  Director Grimes has released a new series of accusations that\n\
  will soon be investigated by a Congressional committee.  Below\n\
  is the message that he sent me.\n\
  \n\
  --------------------------------------------------------------\n\
  Subj: Congressional Investigations\n\
  To: Constantine Dvorak, Undersecretary of Nuclear Safety\n\
  From: Robert Shane, director of the EPA\n\
  \n\
      The EPA has opposed the Security-Restricted Containment\n\
  Facility (SRCF) project from its inception.  We were horrified\n\
  that these facilities would be constructed so close to populated\n\
  areas, and only agreed to sign-off on the project if we were\n\
  allowed to freely examine and monitor the sarcophagi.  But that\n\
  has not happened.  Since then, the DoE has employed any and all\n\
  means to keep EPA agents from visiting the SRCFs, using military\n\
  secrecy, emergency powers, and inter-departmental gag orders to\n" ) );
            query_any( _( "Press any key to continue..." ) );
            reset_terminal();
            print_line( _( " surround the project with an impenetrable thicket of red tape.\n\
  \n\
      Although our agents have not been allowed inside, our atmospheric\n\
  testers in nearby communities have detected high levels of toxins\n\
  and radiation, and we've found dozens of potentially dangerous\n\
  unidentified compounds in the ground water.  We now have\n\
  conclusive evidence that the SRCFs are a threat to the public\n\
  safety.  We are taking these data to state representatives and\n\
  petitioning for a full Congressional inquiry.  They should be\n\
  able to force open your secret vaults, and the world will see\n\
  what you've been hiding.\n\
  \n\
  If you had any hand in this outbreak I hope you rot in hell.\n\
  \n\
  Director of the EPA,\n\
  Robert Shane\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_3_MESS:
            reset_terminal();
            print_line( _( " Subj: CDC: Internal Memo, Standby [2918115]\n\
  To: all SRCF staff\n\
  From:  Ellen Grimes, Director of the CDC\n\
  \n\
      Your site along with many others has been found to be\n\
  contaminated with what we will now refer to as [redacted].\n\
  It is vital that you standby for further orders.  We are\n\
  currently awaiting the President to decide our course of\n\
  action in this national crisis.  You will proceed with fail-\n\
  safe procedures and rig the sarcophagus with C-4 as outlined\n\
  in Publication 4423.  We will send you orders to either detonate\n\
  and seal the sarcophagus or remove the charges.  It is of the\n\
  utmost importance that the facility be sealed immediately when\n\
  the orders are given.  We have been alerted by Homeland Security\n\
  that there are potential terrorist suspects that are being\n\
  detained in connection with the recent national crisis.\n\
  \n\
  Director of the CDC,\n\
  Ellen Grimes\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_SEAL_ORDER:
            reset_terminal();
            print_line( _( " Subj: USARMY: SEAL SRCF [987167]\n\
  To: all SRCF staff\n\
  From:  Major General Cornelius, U.S. Army\n\
  \n\
    As a general warning to all civilian staff: the 10th Mountain\n\
  Division has been assigned to oversee the sealing of the SRCF\n\
  facilities.  By direct order, all non-essential staff must vacate\n\
  at the earliest possible opportunity to prevent potential\n\
  contamination.  Low yield tactical nuclear demolition charges\n\
  will be deployed in the lower tunnels to ensure that recovery\n\
  of hazardous material is impossible.  The Army Corps of Engineers\n\
  will then dump concrete over the rubble so that we can redeploy \n\
  the 10th Mountain into the greater Boston area.\n\
  \n\
  Cornelius,\n\
  Major General, U.S. Army\n\
  Commander of the 10th Mountain Division\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_SEAL:
            g->u.add_memorial_log( pgettext( "memorial_male", "Sealed a Hazardous Material Sarcophagus." ),
                                   pgettext( "memorial_female", "Sealed a Hazardous Material Sarcophagus." ) );
            print_line( _( "Charges Detonated" ) );
            print_line( _( "Backup Generator Power Failing" ) );
            print_line( _( "Evacuate Immediately" ) );
            add_msg( m_warning, _( "Evacuate Immediately!" ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    tripoint p( x, y, g->get_levz() );
                    if( g->m.ter( x, y ) == t_elevator || g->m.ter( x, y ) == t_vat ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                        explosion_handler::explosion( p, 40, 0.7, true );
                    }
                    if( g->m.ter( x, y ) == t_wall_glass ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                    }
                    if( g->m.ter( x, y ) == t_sewage_pipe || g->m.ter( x, y ) == t_sewage ||
                        g->m.ter( x, y ) == t_grate ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                    }
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                        explosion_handler::explosion( p, 50, 0.7, true );
                    }
                }
            }
            options.clear(); // Disable the terminal.
            activate_failure( COMPFAIL_SHUTDOWN );
            break;

        case COMPACT_SRCF_ELEVATOR:
            if( !g->u.has_amount( "sarcophagus_access_code", 1 ) ) {
                print_error( _( "Access code required!" ) );
            } else {
                g->u.use_amount( "sarcophagus_access_code", 1 );
                reset_terminal();
                print_line(
                    _( "\nPower:         Backup Only\nRadiation Level:  Very Dangerous\nOperational:   Overridden\n\n" ) );
                for( int x = 0; x < MAPSIZE_X; x++ ) {
                    for( int y = 0; y < MAPSIZE_Y; y++ ) {
                        if( g->m.ter( x, y ) == t_elevator_control_off ) {
                            g->m.ter_set( x, y, t_elevator_control );

                        }
                    }
                }
            }
            query_any( _( "Press any key..." ) );
            break;

        //irradiates food at t_rad_platform, adds radiation
        case COMPACT_IRRADIATOR: {
            g->u.moves -= 30;
            bool error = false;
            bool platform_exists = false;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.ter( dest ) == t_rad_platform ) {
                    platform_exists = true;
                    if( g->m.i_at( dest ).empty() ) {
                        print_error( _( "ERROR: Processing platform empty." ) );
                    } else {
                        g->u.moves -= 300;
                        for( auto it = g->m.i_at( dest ).begin(); it != g->m.i_at( dest ).end(); ++it ) {
                            // actual food processing
                            if( !it->rotten() && item_controller->has_template( "irradiated_" + it->typeId() ) ) {
                                it->convert( "irradiated_" + it->typeId() );
                            }
                            // critical failure - radiation spike sets off electronic detonators
                            if( it->typeId() == "mininuke" || it->typeId() == "mininuke_act" || it->typeId() == "c4" ) {
                                explosion_handler::explosion( dest, 40 );
                                reset_terminal();
                                print_error( _( "WARNING [409]: Primary sensors offline!" ) );
                                print_error( _( "  >> Initialize secondary sensors:  Geiger profiling..." ) );
                                print_error( _( "  >> Radiation spike detected!\n" ) );
                                print_error( _( "WARNING [912]: Catastrophic malfunction!  Contamination detected! " ) );
                                print_error( _( "EMERGENCY PROCEDURE [1]:  Evacuate.  Evacuate.  Evacuate.\n" ) );
                                sounds::sound( g->u.pos(), 30, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                                               "alarm" );
                                g->m.i_rem( dest, it );
                                g->m.make_rubble( dest );
                                g->m.propagate_field( dest, fd_nuke_gas, 100, 3 );
                                g->m.translate_radius( t_water_pool, t_sewage, 8.0, dest, true );
                                g->m.adjust_radiation( dest, rng( 50, 500 ) );
                                for( const tripoint &radorigin : g->m.points_in_radius( dest, 5 ) ) {
                                    g->m.adjust_radiation( radorigin, rng( 50, 500 ) / ( rl_dist( radorigin,
                                                           dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                                }
                                if( g->m.pl_sees( dest, 10 ) ) {
                                    g->u.irradiate( rng( 50, 250 ) / rl_dist( g->u.pos(), dest ) );
                                } else {
                                    g->u.irradiate( rng( 20, 100 ) / rl_dist( g->u.pos(), dest ) );
                                }
                                query_any( _( "EMERGENCY SHUTDOWN!  Press any key..." ) );
                                error = true;
                                options.clear(); // Disable the terminal.
                                activate_failure( COMPFAIL_SHUTDOWN );
                                break;
                            }
                            g->m.adjust_radiation( dest, rng( 20, 50 ) );
                            for( const tripoint &radorigin : g->m.points_in_radius( dest, 5 ) ) {
                                g->m.adjust_radiation( radorigin, rng( 20, 50 ) / ( rl_dist( radorigin,
                                                       dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                            }
                            // if unshielded, rad source irradiates player directly, reduced by distance to source
                            if( g->m.pl_sees( dest, 10 ) ) {
                                g->u.irradiate( rng( 5, 25 ) / rl_dist( g->u.pos(), dest ) );
                            }
                        }
                        if( !error && platform_exists ) {
                            print_error( _( "PROCESSING...  CYCLE COMPLETE." ) );
                            print_error( _( "GEIGER COUNTER @ PLATFORM: %s mSv/h." ), g->m.get_radiation( dest ) );
                        }
                    }
                }
            }
            if( !platform_exists ) {
                print_error(
                    _( "CRITICAL ERROR... RADIATION PLATFORM UNRESPONSIVE.  COMPLY TO PROCEDURE RP_M_01_rev.03." ) );
            }
            if( !error ) {
                query_any( _( "Press any key..." ) );
            }
            break;
        }

        // geiger counter for irradiator, primary measurement at t_rad_platform, secondary at player loacation
        case COMPACT_GEIGER: {
            g->u.moves -= 30;
            tripoint platform;
            bool source_exists = false;
            int sum_rads = 0;
            int peak_rad = 0;
            int tiles_counted = 0;
            print_error( _( "RADIATION MEASUREMENTS:" ) );
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.ter( dest ) == t_rad_platform ) {
                    source_exists = true;
                    platform = dest;
                }
            }
            if( source_exists ) {
                for( const tripoint &dest : g->m.points_in_radius( platform, 3 ) ) {
                    sum_rads += g->m.get_radiation( dest );
                    tiles_counted ++;
                    if( g->m.get_radiation( dest ) > peak_rad ) {
                        peak_rad = g->m.get_radiation( dest );
                    }
                    sum_rads += g->m.get_radiation( platform );
                    tiles_counted ++;
                    if( g->m.get_radiation( platform ) > peak_rad ) {
                        peak_rad = g->m.get_radiation( platform );
                    }
                }
                print_error( _( "GEIGER COUNTER @ ZONE:... AVG %s mSv/h." ), sum_rads / tiles_counted );
                print_error( _( "GEIGER COUNTER @ ZONE:... MAX %s mSv/h." ), peak_rad );
                print_newline();
            }
            print_error( _( "GEIGER COUNTER @ CONSOLE: .... %s mSv/h." ), g->m.get_radiation( g->u.pos() ) );
            print_error( _( "PERSONAL DOSIMETRY: .... %s mSv." ), g->u.radiation );
            print_newline();
            query_any( _( "Press any key..." ) );
            break;
        }

        // imitates item movement through conveyor belt through 3 different loading/unloading bays
        // ensure only bay of each type in range
        case COMPACT_CONVEYOR: {
            g->u.moves -= 300;
            tripoint loading; // red tile = loading bay
            tripoint unloading; // green tile = unloading bay
            tripoint platform; // radiation platform = middle point
            bool l_exists = false;
            bool u_exists = false;
            bool p_exists = false;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.ter( dest ) == t_rad_platform ) {
                    platform = dest;
                    p_exists = true;
                } else if( g->m.ter( dest ) == t_floor_red ) {
                    loading = dest;
                    l_exists = true;
                } else if( g->m.ter( dest ) == t_floor_green ) {
                    unloading = dest;
                    u_exists = true;
                }
            }
            if( !l_exists || !p_exists || !u_exists ) {
                print_error( _( "Conveyor belt malfunction.  Consult maintenance team." ) );
                query_any( _( "Press any key..." ) );
                break;
            }
            auto items = g->m.i_at( platform );
            if( !items.empty() ) {
                print_line( _( "Moving items: PLATFORM --> UNLOADING BAY." ) );
            } else {
                print_line( _( "No items detected at: PLATFORM." ) );
            }
            for( const auto &it : items ) {
                g->m.add_item_or_charges( unloading, it );
            }
            g->m.i_clear( platform );
            items = g->m.i_at( loading );
            if( !items.empty() ) {
                print_line( _( "Moving items: LOADING BAY --> PLATFORM." ) );
            } else {
                print_line( _( "No items detected at: LOADING BAY." ) );
            }
            for( const auto &it : items ) {
                if( !it.made_of_from_type( LIQUID ) ) {
                    g->m.add_item_or_charges( platform, it );
                }
            }
            g->m.i_clear( loading );
            query_any( _( "Conveyor belt cycle complete.  Press any key..." ) );
            break;
        }
        // toggles reinforced glass shutters open->closed and closed->open depending on their current state
        case COMPACT_SHUTTERS:
            g->u.moves -= 300;
            g->m.translate_radius( t_reinforced_glass_shutter_open, t_reinforced_glass_shutter, 8.0, g->u.pos(),
                                   true, true );
            query_any( _( "Toggling shutters.  Press any key..." ) );
            break;
        // extract radiation source material from irradiator
        case COMPACT_EXTRACT_RAD_SOURCE:
            if( query_yn( _( "Operation irreversible.  Extract radioactive material?" ) ) ) {
                g->u.moves -= 300;
                tripoint platform;
                bool p_exists = false;
                for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                    if( g->m.ter( dest ) == t_rad_platform ) {
                        platform = dest;
                        p_exists = true;
                    }
                }
                if( p_exists ) {
                    g->m.spawn_item( platform, "cobalt_60", rng( 8, 15 ) );
                    g->m.translate_radius( t_rad_platform, t_concrete, 8.0, g->u.pos(), true );
                    remove_option( COMPACT_IRRADIATOR );
                    remove_option( COMPACT_EXTRACT_RAD_SOURCE );
                    query_any( _( "Extraction sequence complete... Press any key." ) );
                } else {
                    query_any( _( "ERROR!  Radiation platform unresponsive... Press any key." ) );
                }
            }
            break;
        // remove shock vent fields; check for existing plutonium generators in radius
        case COMPACT_DEACTIVATE_SHOCK_VENT:
            g->u.moves -= 30;
            bool has_vent = false;
            bool has_generator = false;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.get_field( dest, fd_shock_vent ) != nullptr ) {
                    has_vent = true;
                }
                if( g->m.ter( dest ) == t_plut_generator ) {
                    has_generator = true;
                }
                g->m.remove_field( dest, fd_shock_vent );
            }
            print_line( _( "Initiating POWER-DIAG ver.2.34 ..." ) );
            if( has_vent ) {
                print_error( _( "Short circuit detected!" ) );
                print_error( _( "Short circuit rerouted." ) );
                print_error( _( "Fuse reseted." ) );
                print_error( _( "Ground re-enabled." ) );
            } else {
                print_line( _( "Internal power lines status: 85%% OFFLINE. Reason: DAMAGED." ) );
            }
            print_line(
                _( "External power lines status: 100%% OFFLINE. Reason: NO EXTERNAL POWER DETECTED." ) );
            if( has_generator ) {
                print_line( _( "Backup power status: STANDBY MODE." ) );
            } else {
                print_error( _( "Backup power status: OFFLINE. Reason: UNKNOWN" ) );
            }
            query_any( _( "Press any key..." ) );
            break;

    } // switch (action)
}

void computer::activate_random_failure()
{
    next_attempt = calendar::turn + 45_minutes;
    static const computer_failure default_failure( COMPFAIL_SHUTDOWN );
    const computer_failure &fail = random_entry( failures, default_failure );
    activate_failure( fail.type );
}

void computer::activate_failure( computer_failure_type fail )
{
    bool found_tile = false;
    switch( fail ) {

        case COMPFAIL_NULL: // Unknown action.
        case NUM_COMPUTER_FAILURES: // Suppress compiler warning [-Wswitch]
            break;

        case COMPFAIL_SHUTDOWN:
            for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
                if( g->m.has_flag( "CONSOLE", p ) ) {
                    g->m.ter_set( p, t_console_broken );
                    add_msg( m_bad, _( "The console shuts down." ) );
                    found_tile = true;
                }
            }
            if( found_tile ) {
                break;
            }
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.has_flag( "CONSOLE", x, y ) ) {
                        g->m.ter_set( x, y, t_console_broken );
                        add_msg( m_bad, _( "The console shuts down." ) );
                    }
                }
            }
            break;

        case COMPFAIL_ALARM:
            g->u.add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                   pgettext( "memorial_female", "Set off an alarm." ) );
            sounds::sound( g->u.pos(), 60, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            if( g->get_levz() > 0 && !g->events.queued( EVENT_WANTED ) ) {
                g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, g->u.global_sm_location() );
            }
            break;

        case COMPFAIL_MANHACKS: {
            int num_robots = rng( 4, 8 );
            for( int i = 0; i < num_robots; i++ ) {
                tripoint mp( 0, 0, g->u.posz() );
                int tries = 0;
                do {
                    mp.x = rng( g->u.posx() - 3, g->u.posx() + 3 );
                    mp.y = rng( g->u.posy() - 3, g->u.posy() + 3 );
                    tries++;
                } while( !g->is_empty( mp ) && tries < 10 );
                if( tries != 10 ) {
                    add_msg( m_warning, _( "Manhacks drop from compartments in the ceiling." ) );
                    g->summon_mon( mon_manhack, mp );
                }
            }
        }
        break;

        case COMPFAIL_SECUBOTS: {
            int num_robots = 1;
            for( int i = 0; i < num_robots; i++ ) {
                tripoint mp( 0, 0, g->u.posz() );
                int tries = 0;
                do {
                    mp.x = rng( g->u.posx() - 3, g->u.posx() + 3 );
                    mp.y = rng( g->u.posy() - 3, g->u.posy() + 3 );
                    tries++;
                } while( !g->is_empty( mp ) && tries < 10 );
                if( tries != 10 ) {
                    add_msg( m_warning, _( "Secubots emerge from compartments in the floor." ) );
                    g->summon_mon( mon_secubot, mp );
                }
            }
        }
        break;

        case COMPFAIL_DAMAGE:
            add_msg( m_neutral, _( "The console shocks you." ) );
            if( g->u.is_elec_immune() ) {
                add_msg( m_good, _( "You're protected from electric shocks." ) );
            } else {
                add_msg( m_bad, _( "Your body is damaged by the electric shock!" ) );
                g->u.hurtall( rng( 1, 10 ), nullptr );
            }
            break;

        case COMPFAIL_PUMP_EXPLODE:
            add_msg( m_warning, _( "The pump explodes!" ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        tripoint p( x, y, g->get_levz() );
                        g->m.make_rubble( p );
                        explosion_handler::explosion( p, 10 );
                    }
                }
            }
            break;

        case COMPFAIL_PUMP_LEAK:
            add_msg( m_warning, _( "Sewage leaks!" ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        point p( x, y );
                        int leak_size = rng( 4, 10 );
                        for( int i = 0; i < leak_size; i++ ) {
                            std::vector<point> next_move;
                            if( g->m.passable( p.x, p.y - 1 ) ) {
                                next_move.push_back( point( p.x, p.y - 1 ) );
                            }
                            if( g->m.passable( p.x + 1, p.y ) ) {
                                next_move.push_back( point( p.x + 1, p.y ) );
                            }
                            if( g->m.passable( p.x, p.y + 1 ) ) {
                                next_move.push_back( point( p.x, p.y + 1 ) );
                            }
                            if( g->m.passable( p.x - 1, p.y ) ) {
                                next_move.push_back( point( p.x - 1, p.y ) );
                            }

                            if( next_move.empty() ) {
                                i = leak_size;
                            } else {
                                p = random_entry( next_move );
                                g->m.ter_set( p.x, p.y, t_sewage );
                            }
                        }
                    }
                }
            }
            break;

        case COMPFAIL_AMIGARA:
            g->events.add( EVENT_AMIGARA, calendar::turn + 30_seconds );
            g->u.add_effect( effect_amigara, 2_minutes );
            explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                          10,
                                          0.7, false, 10 );
            explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                          10,
                                          0.7, false, 10 );
            remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPFAIL_DESTROY_BLOOD:
            print_error( _( "ERROR: Disruptive Spin" ) );
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_centrifuge ) {
                    map_stack items = g->m.i_at( dest );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place sample in centrifuge." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
                    } else if( items.only_item().typeId() != "vacutainer" ) {
                        print_error( _( "ERROR: Please use blood-contained samples." ) );
                    } else if( items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Blood draw kit, empty." ) );
                    } else if( items.only_item().contents.front().typeId() != "blood" ) {
                        print_error( _( "ERROR: Please only use blood samples." ) );
                    } else {
                        print_error( _( "ERROR: Blood sample destroyed." ) );
                        g->m.i_clear( dest );
                    }
                }
            }
            inp_mngr.wait_for_any_key();
            break;

        case COMPFAIL_DESTROY_DATA:
            print_error( _( "ERROR: ACCESSING DATA MALFUNCTION" ) );
            for( int x = 0; x < SEEX * 2; x++ ) {
                for( int y = 0; y < SEEY * 2; y++ ) {
                    if( g->m.ter( x, y ) == t_floor_blue ) {
                        map_stack items = g->m.i_at( x, y );
                        if( items.empty() ) {
                            print_error( _( "ERROR: Please place memory bank in scan area." ) );
                        } else if( items.size() > 1 ) {
                            print_error( _( "ERROR: Please only scan one item at a time." ) );
                        } else if( items.only_item().typeId() != "usb_drive" ) {
                            print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                        } else if( items.only_item().contents.empty() ) {
                            print_error( _( "ERROR: Memory bank is empty." ) );
                        } else {
                            print_error( _( "ERROR: Data bank destroyed." ) );
                            g->m.i_clear( x, y );
                        }
                    }
                }
            }
            inp_mngr.wait_for_any_key();
            break;

    }// switch (fail)
=======
void computer::serialize( JsonOut &jout ) const
{
    jout.start_object();
    jout.member( "name", name );
    jout.member( "mission", mission_id );
    jout.member( "security", security );
    jout.member( "alerts", alerts );
    jout.member( "next_attempt", next_attempt );
    jout.member( "options", options );
    jout.member( "failures", failures );
    jout.member( "access_denied", access_denied );
    jout.end_object();
}

void computer::deserialize( JsonIn &jin )
{
    if( jin.test_string() ) {
        load_legacy_data( jin.get_string() );
    } else {
        const JsonObject jo = jin.get_object();
        jo.read( "name", name );
        jo.read( "mission", mission_id );
        jo.read( "security", security );
        jo.read( "alerts", alerts );
        jo.read( "next_attempt", next_attempt );
        jo.read( "options", options );
        jo.read( "failures", failures );
        jo.read( "access_denied", access_denied );
    }
>>>>>>> upstream/master
}

void computer::remove_option( computer_action const action )
{
    for( auto it = options.begin(); it != options.end(); ++it ) {
        if( it->action == action ) {
            options.erase( it );
            break;
        }
    }
}

static computer_action computer_action_from_legacy_enum( const int val )
{
    switch( val ) {
        // Used to migrate old saves. Do not change the numbers!
        // *INDENT-OFF*
        default: return COMPACT_NULL;
        case 0: return COMPACT_NULL;
        case 1: return COMPACT_OPEN;
        case 2: return COMPACT_LOCK;
        case 3: return COMPACT_UNLOCK;
        case 4: return COMPACT_TOLL;
        case 5: return COMPACT_SAMPLE;
        case 6: return COMPACT_RELEASE;
        case 7: return COMPACT_RELEASE_BIONICS;
        case 8: return COMPACT_TERMINATE;
        case 9: return COMPACT_PORTAL;
        case 10: return COMPACT_CASCADE;
        case 11: return COMPACT_RESEARCH;
        case 12: return COMPACT_MAPS;
        case 13: return COMPACT_MAP_SEWER;
        case 14: return COMPACT_MAP_SUBWAY;
        // options with action enum 15 are removed in load_legacy_data()
        case 16: return COMPACT_MISS_DISARM;
        case 17: return COMPACT_LIST_BIONICS;
        case 18: return COMPACT_ELEVATOR_ON;
        case 19: return COMPACT_AMIGARA_LOG;
        case 20: return COMPACT_AMIGARA_START;
        case 21: return COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER;
        case 22: return COMPACT_REPEATER_MOD;
        case 23: return COMPACT_DOWNLOAD_SOFTWARE;
        case 24: return COMPACT_BLOOD_ANAL;
        case 25: return COMPACT_DATA_ANAL;
        case 26: return COMPACT_DISCONNECT;
        case 27: return COMPACT_EMERG_MESS;
        case 28: return COMPACT_EMERG_REF_CENTER;
        case 29: return COMPACT_TOWER_UNRESPONSIVE;
        case 30: return COMPACT_SR1_MESS;
        case 31: return COMPACT_SR2_MESS;
        case 32: return COMPACT_SR3_MESS;
        case 33: return COMPACT_SR4_MESS;
        case 34: return COMPACT_SRCF_1_MESS;
        case 35: return COMPACT_SRCF_2_MESS;
        case 36: return COMPACT_SRCF_3_MESS;
        case 37: return COMPACT_SRCF_SEAL_ORDER;
        case 38: return COMPACT_SRCF_SEAL;
        case 39: return COMPACT_SRCF_ELEVATOR;
        case 40: return COMPACT_OPEN_DISARM;
        case 41: return COMPACT_UNLOCK_DISARM;
        case 42: return COMPACT_RELEASE_DISARM;
        case 43: return COMPACT_IRRADIATOR;
        case 44: return COMPACT_GEIGER;
        case 45: return COMPACT_CONVEYOR;
        case 46: return COMPACT_SHUTTERS;
        case 47: return COMPACT_EXTRACT_RAD_SOURCE;
        case 48: return COMPACT_DEACTIVATE_SHOCK_VENT;
        case 49: return COMPACT_RADIO_ARCHIVE;
        // *INDENT-ON*
    }
}

static computer_failure_type computer_failure_type_from_legacy_enum( const int val )
{
    switch( val ) {
        // Used to migrate old saves. Do not change the numbers!
        // *INDENT-OFF*
        default: return COMPFAIL_NULL;
        case 0: return COMPFAIL_NULL;
        case 1: return COMPFAIL_SHUTDOWN;
        case 2: return COMPFAIL_ALARM;
        case 3: return COMPFAIL_MANHACKS;
        case 4: return COMPFAIL_SECUBOTS;
        case 5: return COMPFAIL_DAMAGE;
        case 6: return COMPFAIL_PUMP_EXPLODE;
        case 7: return COMPFAIL_PUMP_LEAK;
        case 8: return COMPFAIL_AMIGARA;
        case 9: return COMPFAIL_DESTROY_BLOOD;
        case 10: return COMPFAIL_DESTROY_DATA;
        // *INDENT-ON*
    }
}

namespace io
{
template<>
std::string enum_to_string<computer_action>( const computer_action act )
{
    switch( act ) {
        // *INDENT-OFF*
        case COMPACT_NULL: return "null";
        case COMPACT_AMIGARA_LOG: return "amigara_log";
        case COMPACT_AMIGARA_START: return "amigara_start";
        case COMPACT_BLOOD_ANAL: return "blood_anal";
        case COMPACT_CASCADE: return "cascade";
        case COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER: return "complete_disable_external_power";
        case COMPACT_CONVEYOR: return "conveyor";
        case COMPACT_DATA_ANAL: return "data_anal";
        case COMPACT_DEACTIVATE_SHOCK_VENT: return "deactivate_shock_vent";
        case COMPACT_DISCONNECT: return "disconnect";
        case COMPACT_DOWNLOAD_SOFTWARE: return "download_software";
        case COMPACT_ELEVATOR_ON: return "elevator_on";
        case COMPACT_EMERG_MESS: return "emerg_mess";
        case COMPACT_EMERG_REF_CENTER: return "emerg_ref_center";
        case COMPACT_EXTRACT_RAD_SOURCE: return "extract_rad_source";
        case COMPACT_GEIGER: return "geiger";
        case COMPACT_IRRADIATOR: return "irradiator";
        case COMPACT_LIST_BIONICS: return "list_bionics";
        case COMPACT_LOCK: return "lock";
        case COMPACT_MAP_SEWER: return "map_sewer";
        case COMPACT_MAP_SUBWAY: return "map_subway";
        case COMPACT_MAPS: return "maps";
        case COMPACT_MISS_DISARM: return "miss_disarm";
        case COMPACT_OPEN: return "open";
        case COMPACT_OPEN_DISARM: return "open_disarm";
        case COMPACT_PORTAL: return "portal";
        case COMPACT_RADIO_ARCHIVE: return "radio_archive";
        case COMPACT_RELEASE: return "release";
        case COMPACT_RELEASE_BIONICS: return "release_bionics";
        case COMPACT_RELEASE_DISARM: return "release_disarm";
        case COMPACT_REPEATER_MOD: return "repeater_mod";
        case COMPACT_RESEARCH: return "research";
        case COMPACT_SAMPLE: return "sample";
        case COMPACT_SHUTTERS: return "shutters";
        case COMPACT_SR1_MESS: return "sr1_mess";
        case COMPACT_SR2_MESS: return "sr2_mess";
        case COMPACT_SR3_MESS: return "sr3_mess";
        case COMPACT_SR4_MESS: return "sr4_mess";
        case COMPACT_SRCF_1_MESS: return "srcf_1_mess";
        case COMPACT_SRCF_2_MESS: return "srcf_2_mess";
        case COMPACT_SRCF_3_MESS: return "srcf_3_mess";
        case COMPACT_SRCF_ELEVATOR: return "srcf_elevator";
        case COMPACT_SRCF_SEAL: return "srcf_seal";
        case COMPACT_SRCF_SEAL_ORDER: return "srcf_seal_order";
        case COMPACT_TERMINATE: return "terminate";
        case COMPACT_TOLL: return "toll";
        case COMPACT_TOWER_UNRESPONSIVE: return "tower_unresponsive";
        case COMPACT_UNLOCK: return "unlock";
        case COMPACT_UNLOCK_DISARM: return "unlock_disarm";
        // *INDENT-OFF*
        case NUM_COMPUTER_ACTIONS:
            break;
    }
    debugmsg( "Invalid computer_action" );
    abort();
}

template<>
std::string enum_to_string<computer_failure_type>( const computer_failure_type fail )
{
    switch( fail ){
        // *INDENT-OFF*
        case COMPFAIL_NULL: return "null";
        case COMPFAIL_ALARM: return "alarm";
        case COMPFAIL_AMIGARA: return "amigara";
        case COMPFAIL_DAMAGE: return "damage";
        case COMPFAIL_DESTROY_BLOOD: return "destroy_blood";
        case COMPFAIL_DESTROY_DATA: return "destroy_data";
        case COMPFAIL_MANHACKS: return "manhacks";
        case COMPFAIL_PUMP_EXPLODE: return "pump_explode";
        case COMPFAIL_PUMP_LEAK: return "pump_leak";
        case COMPFAIL_SECUBOTS: return "secubots";
        case COMPFAIL_SHUTDOWN: return "shutdown";
        // *INDENT-ON*
        case NUM_COMPUTER_FAILURES:
            break;
    }
    debugmsg( "Invalid computer_failure_type" );
    abort();
}
} // namespace io

template<>
struct enum_traits<computer_action> {
    static constexpr computer_action last = NUM_COMPUTER_ACTIONS;
};

template<>
struct enum_traits<computer_failure_type> {
    static constexpr computer_failure_type last = NUM_COMPUTER_FAILURES;
};

computer_option computer_option::from_json( const JsonObject &jo )
{
    translation name;
    jo.read( "name", name );
    const computer_action action = jo.get_enum_value<computer_action>( "action" );
    const int sec = jo.get_int( "security", 0 );
    return computer_option( name.translated(), action, sec );
}

computer_failure computer_failure::from_json( const JsonObject &jo )
{
    const computer_failure_type type = jo.get_enum_value<computer_failure_type>( "action" );
    return computer_failure( type );
}
