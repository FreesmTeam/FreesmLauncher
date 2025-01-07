if(UNIX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_search_module(SCDOC scdoc)
        if(SCDOC_FOUND)
            pkg_get_variable(SCDOC_SCDOC scdoc scdoc)
        endif()
    endif()
endif()

set(Launcher_CommonName "FreesmLauncher")
set(Launcher_DisplayName "Freesm Launcher")

set(Launcher_Name "${Launcher_CommonName}" PARENT_SCOPE)
set(Launcher_DisplayName "${Launcher_DisplayName}" PARENT_SCOPE)

set(Launcher_Copyright "© 2024-2025 Freesm Launcher Contributors\\n© 2022-2024 Prism Launcher Contributors\\n© 2021-2022 PolyMC Contributors\\n© 2012-2021 MultiMC Contributors")
set(Launcher_Copyright_Mac "© 2024-2025 Freesm Launcher Contributors\\n© 2022-2024 Prism Launcher Contributors, © 2021-2022 PolyMC Contributors and © 2012-2021 MultiMC Contributors" PARENT_SCOPE)
set(Launcher_Copyright "${Launcher_Copyright}" PARENT_SCOPE)
set(Launcher_Domain "https://github.com/FreesmTeam/FreesmLauncher" PARENT_SCOPE)
set(Launcher_UserAgent "${Launcher_CommonName}/${Launcher_VERSION_NAME}" PARENT_SCOPE)
set(Launcher_ConfigFile "freesmlauncher.cfg" PARENT_SCOPE)
set(Launcher_Git "https://github.com/FreesmTeam/FreesmLauncher" PARENT_SCOPE)
set(Launcher_DesktopFileName "org.freesmTeam.freesmlauncher.desktop" PARENT_SCOPE)
set(Launcher_SVGFileName "org.freesmTeam.freesmlauncher.svg" PARENT_SCOPE)

set(Launcher_Desktop "program_info/org.freesmTeam.freesmlauncher.desktop" PARENT_SCOPE)
set(Launcher_mrpack_MIMEInfo "program_info/modrinth-mrpack-mime.xml" PARENT_SCOPE)
set(Launcher_MetaInfo "program_info/org.freesmTeam.freesmlauncher.metainfo.xml" PARENT_SCOPE)
set(Launcher_SVG "program_info/org.freesmTeam.freesmlauncher.svg" PARENT_SCOPE)
set(Launcher_Branding_ICNS "program_info/freesmlauncher.icns" PARENT_SCOPE)
set(Launcher_Branding_ICO "program_info/freesmlauncher.ico")
set(Launcher_Branding_ICO "${Launcher_Branding_ICO}" PARENT_SCOPE)
set(Launcher_Branding_WindowsRC "program_info/freesmlauncher.rc" PARENT_SCOPE)
set(Launcher_Branding_LogoQRC "program_info/freesmlauncher.qrc" PARENT_SCOPE)

set(Launcher_Portable_File "program_info/portable.txt" PARENT_SCOPE)

configure_file(org.freesmTeam.freesmlauncher.desktop.in org.freesmTeam.freesmlauncher.desktop)
configure_file(org.freesmTeam.freesmlauncher.metainfo.xml.in org.freesmTeam.freesmlauncher.metainfo.xml)
configure_file(freesmlauncher.rc.in freesmlauncher.rc @ONLY)
configure_file(freesmlauncher.manifest.in freesmlauncher.manifest @ONLY)
configure_file(freesmlauncher.ico freesmlauncher.ico COPYONLY)

if(MSVC)
    set(Launcher_MSVC_Redist_NSIS_Section [=[
!ifdef haveNScurl
Section "Visual Studio Runtime"
    Var /GLOBAL vc_redist_exe
    ${If} ${IsNativeARM64}
        StrCpy $vc_redist_exe "vc_redist.arm64.exe"
    ${Else}
        StrCpy $vc_redist_exe "vc_redist.x64.exe"
    ${EndIf}
    DetailPrint 'Downloading Microsoft Visual C++ Redistributable...'
    NScurl::http GET "https://aka.ms/vs/17/release/$vc_redist_exe" "$INSTDIR\vc_redist\$vc_redist_exe" /INSIST /CANCEL /Zone.Identifier /END
    Pop $0
    ${If} $0 == "OK"
        DetailPrint "Download successful"
        ExecWait "$INSTDIR\vc_redist\$vc_redist_exe /install /passive /norestart"
    ${Else}
        DetailPrint "Download failed with error $0"
    ${EndIf}
SectionEnd
!endif
]=])
endif()

configure_file(win_install.nsi.in win_install.nsi @ONLY)

if(SCDOC_FOUND)
    set(in_scd "${CMAKE_CURRENT_SOURCE_DIR}/freesmlauncher.6.scd")
    set(out_man "${CMAKE_CURRENT_BINARY_DIR}/freesmlauncher.6")
    add_custom_command(
        DEPENDS "${in_scd}"
        OUTPUT "${out_man}"
        COMMAND ${SCDOC_SCDOC} < "${in_scd}" > "${out_man}"
    )
    add_custom_target(man ALL DEPENDS ${out_man})
    set(Launcher_ManPage "program_info/freesmlauncher.6" PARENT_SCOPE)
endif()
