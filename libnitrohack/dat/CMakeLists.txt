# dat/CMakeLists.txt: generate game data files
include (CMakeParseArguments)

configure_file (history history COPYONLY)

# makedefs -d
add_custom_command (OUTPUT data
                    COMMAND makedefs
                    ARGS -d ${LNH_DAT}/data.base data
                    MAIN_DEPENDENCY data.base
                    DEPENDS makedefs)
# makedefs -e
add_custom_command (OUTPUT dungeon.pdf
                    COMMAND makedefs
                    ARGS -e ${LNH_DAT}/dungeon.def dungeon.pdf
                    MAIN_DEPENDENCY dungeon.def
                    DEPENDS makedefs)
# makedefs -q
add_custom_command (OUTPUT quest.dat
                    COMMAND makedefs
                    ARGS -q ${LNH_DAT}/quest.txt quest.dat
                    MAIN_DEPENDENCY quest.txt
                    DEPENDS makedefs)
# makedefs -r
add_custom_command (OUTPUT rumors
                    COMMAND makedefs
                    ARGS -r ${LNH_DAT}/rumors.tru
                            ${LNH_DAT}/rumors.fal rumors
                    DEPENDS makedefs rumors.tru rumors.fal)
# makedefs -h
add_custom_command (OUTPUT oracles
                    COMMAND makedefs
                    ARGS -h ${LNH_DAT}/oracles.txt oracles
                    MAIN_DEPENDENCY oracles.txt
                    DEPENDS makedefs)

add_custom_command (OUTPUT dungeon
                    COMMAND dgn_comp
                    ARGS dungeon.pdf
                    MAIN_DEPENDENCY dungeon.pdf
                    DEPENDS dgn_comp)

# Level files
macro (level_file)
    cmake_parse_arguments (LEVEL_FILE "" "INPUT" "OUTPUTS" ${ARGN})
    set (REAL_OUTPUTS)
    foreach (OUTPUT ${LEVEL_FILE_OUTPUTS})
        list (APPEND REAL_OUTPUTS ${OUTPUT}.lev)
    endforeach (OUTPUT)
    list (APPEND COMPILED_LEVELS ${REAL_OUTPUTS})
    add_custom_command (OUTPUT ${REAL_OUTPUTS}
                        COMMAND lev_comp
                        ARGS -v ${LNH_DAT}/${LEVEL_FILE_INPUT}
                        MAIN_DEPENDENCY ${LEVEL_FILE_INPUT}
                        DEPENDS lev_comp)
endmacro (level_file)

macro (quest_file BASE INPUT)
    level_file (INPUT ${INPUT}
                OUTPUTS
                ${BASE}-strt ${BASE}-fila ${BASE}-loca ${BASE}-filb
                ${BASE}-goal)
endmacro (quest_file)

# Dungeons of Doom
level_file (INPUT oracle.des OUTPUTS oracle)
level_file (INPUT bigroom.des
            OUTPUTS
            bigrm-1 bigrm-2  bigrm-3 bigrm-4 bigrm-5 bigrm-6 bigrm-7 bigrm-8
            bigrm-9 bigrm-10 bigrm-11)
level_file (INPUT medusa.des OUTPUTS medusa-1 medusa-2 medusa-3 medusa-4)
level_file (INPUT castle.des OUTPUTS castle-1 castle-2 castle-3)

# Advent Calendar
level_file (INPUT advcal.des OUTPUTS advcal)

# Gnomish Mines
level_file (INPUT mines.des
            OUTPUTS
            minefill minend-1 minend-2 minend-3
            minetn-1 minetn-2 minetn-3 minetn-4 minetn-5 minetn-6 minetn-7
            minetn-8)

# Sokoban
level_file (INPUT sokoban.des
            OUTPUTS
            soko1-1  soko1-2  soko1-3  soko1-4  soko1-5  soko1-6
            soko2-1  soko2-2  soko2-3  soko2-4  soko2-5  soko2-6  soko2-7
            soko2-8  soko2-9  soko2-10 soko2-11
            soko3-1  soko3-2  soko3-3  soko3-4  soko3-5  soko3-6  soko3-7
            soko3-8  soko3-9  soko3-10 soko3-11 soko3-12 soko3-13 soko3-14
            soko3-15 soko3-16 soko3-17 soko3-18)

# Town
level_file (INPUT town.des OUTPUTS town-1 town-2 townfill)

# Nymph
level_file (INPUT nymph.des OUTPUTS nymph)

# Quests
quest_file (Arc Arch.des)
quest_file (Bar Barb.des)
quest_file (Cav Caveman.des)
quest_file (Con Convict.des)
quest_file (Hea Healer.des)
quest_file (Kni Knight.des)
quest_file (Mon Monk.des)
quest_file (Pri Priest.des)
quest_file (Ran Ranger.des)
quest_file (Rog Rogue.des)
quest_file (Sam Samurai.des)
quest_file (Tou Tourist.des)
quest_file (Val Valkyrie.des)
quest_file (Wiz Wizard.des)

# Knox
level_file (INPUT knox.des OUTPUTS knox-1 knox-2 knox-3)

# Black Market
level_file (INPUT blkmar.des OUTPUTS blkmar)

# Gehennom
level_file (INPUT gehennom.des
            OUTPUTS
            asmod-1 asmod-2 baalz-1 baalz-2 hellfill juiblex orcus-1 orcus-2
            sanctum valley-1 valley-2)
level_file (INPUT heck.des
            OUTPUTS
            heck-a-1 heck-a-2 heck-a-3 heck-a-4 heck-a-5 heck-a-6 heck-a-7
            heck-b-1 heck-b-2 heck-b-3 heck-b-4 heck-b-5 heck-b-6 heck-b-7
            heck-c-1 heck-c-2 heck-c-3 heck-c-4 heck-c-5 heck-c-6 heck-c-7
            heck-c-8
            heck-d-1 heck-d-2 heck-d-3 heck-d-4 heck-d-5 heck-d-6 heck-d-7
            heck-d-8 heck-d-9
            heck-e-1 heck-e-2 heck-e-3 heck-e-4 heck-e-5 heck-e-6 heck-e-7
            heck-e-8 heck-e-9)
level_file (INPUT yendor.des OUTPUTS fakewiz1 fakewiz2 wizard1 wizard2 wizard3)

# Vlad's Tower
level_file (INPUT tower.des OUTPUTS tower1 tower2 tower3)

# Dragon Caves
level_file (INPUT dragons.des OUTPUTS drgnA drgnB drgnC drgnD)

# End Game
level_file (INPUT endgame.des OUTPUTS earth air fire water astral)

set (NHDAT_SRC dungeon quest.dat rumors oracles ${COMPILED_LEVELS} history data)

add_custom_command (OUTPUT nhdat
                    COMMAND dlb
                    ARGS cf nhdat ${NHDAT_SRC}
                    DEPENDS dlb ${NHDAT_SRC})

# pseudo-target: depend on all other generated data
add_custom_target (nitrohack_data_files ALL
                   DEPENDS nhdat)

install(FILES ${LNH_DAT_GEN}/nhdat
        DESTINATION ${DATADIR})
install(FILES license
        DESTINATION ${DATADIR})
