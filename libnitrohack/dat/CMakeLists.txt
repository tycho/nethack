# dat/CMakeLists.txt: generate game data files

set (LEVEL_DESCRIPTIONS
    ${LNH_DAT}/Arch.des
    ${LNH_DAT}/Barb.des
    ${LNH_DAT}/bigroom.des
    ${LNH_DAT}/blkmar.des
    ${LNH_DAT}/castle.des
    ${LNH_DAT}/Caveman.des
    ${LNH_DAT}/Convict.des
    ${LNH_DAT}/dragons.des
    ${LNH_DAT}/endgame.des
    ${LNH_DAT}/gehennom.des
    ${LNH_DAT}/Healer.des
    ${LNH_DAT}/heck.des
    ${LNH_DAT}/Knight.des
    ${LNH_DAT}/knox.des
    ${LNH_DAT}/medusa.des
    ${LNH_DAT}/mines.des
    ${LNH_DAT}/nymph.des
    ${LNH_DAT}/Monk.des
    ${LNH_DAT}/oracle.des
    ${LNH_DAT}/Priest.des
    ${LNH_DAT}/Ranger.des
    ${LNH_DAT}/Rogue.des
    ${LNH_DAT}/Samurai.des
    ${LNH_DAT}/sokoban.des
    ${LNH_DAT}/Tourist.des
    ${LNH_DAT}/tower.des
    ${LNH_DAT}/town.des
    ${LNH_DAT}/Valkyrie.des
    ${LNH_DAT}/Wizard.des
    ${LNH_DAT}/yendor.des 
    )

# the compiled levels that will be added to nhdat
set (COMPILED_LEVELS
    air.lev      Arc-fila.lev Arc-filb.lev Arc-goal.lev
    Arc-loca.lev Arc-strt.lev asmod-1.lev  asmod-2.lev
    astral.lev   baalz-1.lev  baalz-2.lev
    Bar-fila.lev Bar-filb.lev Bar-goal.lev
    Bar-loca.lev Bar-strt.lev blkmar.lev
    bigrm-1.lev  bigrm-2.lev  bigrm-3.lev  bigrm-4.lev
    bigrm-5.lev  bigrm-6.lev  bigrm-7.lev  bigrm-8.lev
    bigrm-9.lev  bigrm-10.lev bigrm-11.lev
    castle-1.lev castle-2.lev castle-3.lev
    Cav-fila.lev Cav-filb.lev Cav-goal.lev Cav-loca.lev
    Cav-strt.lev
    Con-fila.lev Con-filb.lev Con-goal.lev Con-loca.lev
    Con-strt.lev
    drgnA.lev    drgnB.lev    drgnC.lev    drgnD.lev
    earth.lev    fakewiz1.lev fakewiz2.lev
    fire.lev     Hea-fila.lev Hea-filb.lev Hea-goal.lev
    Hea-loca.lev Hea-strt.lev
    heck-a-1.lev heck-a-2.lev heck-a-3.lev heck-a-4.lev
    heck-a-5.lev heck-a-6.lev heck-a-7.lev
    heck-b-1.lev heck-b-2.lev heck-b-3.lev heck-b-4.lev
    heck-b-5.lev heck-b-6.lev heck-b-7.lev
    heck-c-1.lev heck-c-2.lev heck-c-3.lev heck-c-4.lev
    heck-c-5.lev heck-c-6.lev heck-c-7.lev heck-c-8.lev
    heck-d-1.lev heck-d-2.lev heck-d-3.lev heck-d-4.lev
    heck-d-5.lev heck-d-6.lev heck-d-7.lev heck-d-8.lev
    heck-d-9.lev
    heck-e-1.lev heck-e-2.lev heck-e-3.lev heck-e-4.lev
    heck-e-5.lev heck-e-6.lev heck-e-7.lev heck-e-8.lev
    heck-e-9.lev
    hellfill.lev
    juiblex.lev  Kni-fila.lev
    Kni-filb.lev Kni-goal.lev Kni-loca.lev Kni-strt.lev
    knox-1.lev   knox-2.lev   knox-3.lev
    medusa-1.lev medusa-2.lev medusa-3.lev medusa-4.lev
    minefill.lev
    minend-1.lev minend-2.lev minend-3.lev minetn-1.lev
    minetn-2.lev minetn-3.lev minetn-4.lev minetn-5.lev
    minetn-6.lev minetn-7.lev minetn-8.lev
    Mon-fila.lev Mon-filb.lev
    Mon-goal.lev Mon-loca.lev Mon-strt.lev
    nymph.lev    oracle.lev   orcus-1.lev  orcus-2.lev
    Pri-fila.lev Pri-filb.lev Pri-goal.lev
    Pri-loca.lev Pri-strt.lev Ran-fila.lev Ran-filb.lev
    Ran-goal.lev Ran-loca.lev Ran-strt.lev Rog-fila.lev
    Rog-filb.lev Rog-goal.lev Rog-loca.lev Rog-strt.lev
    Sam-fila.lev Sam-filb.lev Sam-goal.lev Sam-loca.lev
    Sam-strt.lev sanctum.lev
    soko1-1.lev  soko1-2.lev  soko1-3.lev  soko1-4.lev
    soko1-5.lev  soko1-6.lev
    soko2-1.lev  soko2-2.lev  soko2-3.lev  soko2-4.lev
    soko2-5.lev  soko2-6.lev  soko2-7.lev  soko2-8.lev
    soko2-9.lev  soko2-10.lev soko2-11.lev
    soko3-1.lev  soko3-2.lev  soko3-3.lev  soko3-4.lev
    soko3-5.lev  soko3-6.lev  soko3-7.lev  soko3-8.lev
    soko3-9.lev  soko3-10.lev soko3-11.lev soko3-12.lev
    soko3-13.lev soko3-14.lev soko3-15.lev soko3-16.lev
    soko3-17.lev soko3-18.lev
    Tou-fila.lev Tou-filb.lev
    Tou-goal.lev Tou-loca.lev Tou-strt.lev tower1.lev
    tower2.lev   tower3.lev
    town-1.lev   town-2.lev   townfill.lev
    Val-fila.lev Val-filb.lev
    Val-goal.lev valley-1.lev valley-2.lev
    Val-loca.lev Val-strt.lev
    water.lev    wizard1.lev  wizard2.lev  wizard3.lev
    Wiz-fila.lev Wiz-filb.lev Wiz-goal.lev Wiz-loca.lev
    Wiz-strt.lev
    )

set (NHDAT_SRC dungeon quest.dat rumors oracles ${COMPILED_LEVELS} history data)

configure_file(${LNH_DAT}/history history COPYONLY)

get_property(MAKEDEFS_BIN TARGET makedefs PROPERTY LOCATION)
get_property(DGN_COMP_BIN TARGET dgn_comp PROPERTY LOCATION)
get_property(LEV_COMP_BIN TARGET lev_comp PROPERTY LOCATION)
get_property(DLB_BIN TARGET dlb PROPERTY LOCATION)


# makedefs -d
add_custom_command (OUTPUT data
                    COMMAND ${MAKEDEFS_BIN}
                    ARGS -d ${LNH_DAT}/data.base data
                    DEPENDS makedefs)
# makedefs -e 
add_custom_command (OUTPUT dungeon.pdf
                    COMMAND ${MAKEDEFS_BIN}
                    ARGS -e ${LNH_DAT}/dungeon.def dungeon.pdf
                    DEPENDS makedefs)
# makedefs -q
add_custom_command (OUTPUT quest.dat
                    COMMAND ${MAKEDEFS_BIN}
                    ARGS -q ${LNH_DAT}/quest.txt quest.dat
                    DEPENDS makedefs)
# makedefs -r
add_custom_command (OUTPUT rumors
                    COMMAND ${MAKEDEFS_BIN}
                    ARGS -r ${LNH_DAT}/rumors.tru
                            ${LNH_DAT}/rumors.fal rumors
                    DEPENDS makedefs)
# makedefs -h
add_custom_command (OUTPUT oracles
                    COMMAND ${MAKEDEFS_BIN}
                    ARGS -h ${LNH_DAT}/oracles.txt oracles
                    DEPENDS makedefs)

add_custom_command (OUTPUT dungeon
                    COMMAND ${DGN_COMP_BIN}
                    ARGS ${LNH_DAT_GEN}/dungeon.pdf
                    DEPENDS dungeon.pdf)

add_custom_command (OUTPUT nhdat
                    COMMAND ${LEV_COMP_BIN}
                    ARGS -v ${LEVEL_DESCRIPTIONS}
                    COMMAND ${DLB_BIN}
                    ARGS cf nhdat ${NHDAT_SRC}
                    DEPENDS lev_comp dungeon quest.dat rumors oracles data ${LEVEL_DESCRIPTIONS})

# pseudo-target: depend on all other generated data
add_custom_target (nitrohack_data_files ALL
                   DEPENDS nhdat)

install(FILES ${LNH_DAT_GEN}/nhdat
        DESTINATION ${DATADIR})
install(FILES license
        DESTINATION ${DATADIR})
