function(TargetLinkInterfaceTwoLibs targetName debugLibsArray releaseLibsArray)
	
	list(LENGTH ${debugLibsArray} ARRAY_LENGTH)
	math(EXPR LAST_INDEX "${ARRAY_LENGTH} - 1")
	
	foreach(INDEX RANGE ${LAST_INDEX})	
		list(GET ${debugLibsArray}   ${INDEX} ELEMENT1)
		list(GET ${releaseLibsArray} ${INDEX} ELEMENT2)
		#message(STATUS "TargetLinkTwoLibs ${INDEX}: ${ELEMENT1} - ${ELEMENT2}")		
		target_link_libraries(${targetName} INTERFACE debug ${ELEMENT1} optimized ${ELEMENT2})
	endforeach()
	
endfunction()	



function(SettingMFC i_targetName)
	set(targetName ${i_targetName})
	message(STATUS "$targetName=${targetName}")		
	set_target_properties(${targetName} PROPERTIES
		COMPILE_DEFINITIONS "$<$<CONFIG:Debug>:CMAKE_MFC_FLAG=2>; $<$<CONFIG:Release>:CMAKE_MFC_FLAG=1>"
	)
	target_compile_definitions(${targetName} PRIVATE 
		$<$<CONFIG:Debug>:_AFXDLL>   
	)
endfunction()


function(CollectHeaderFun basepath sourcename inoutvar)
    set(namevar ${basepath}/${sourcename})
    file(GLOB DetailFiles "${namevar}/*.h" "${namevar}/*.hpp")

    source_group("include" FILES ${DetailFiles})

    list(APPEND ${inoutvar} ${DetailFiles})  
    set(${inoutvar} ${${inoutvar}} PARENT_SCOPE)
endfunction()


function(CollectGroupHeaderFun basepath sourcename inoutvar)
    set(namevar ${basepath}/${sourcename})
    file(GLOB DetailFiles "${namevar}/*.h" "${namevar}/*.hpp")

    source_group(${sourcename} FILES ${DetailFiles})

    list(APPEND ${inoutvar} ${DetailFiles})  
    set(${inoutvar} ${${inoutvar}} PARENT_SCOPE)
endfunction()


function(CollectSrcFun basepath sourcename inoutvar)
    set(namevar ${basepath}/${sourcename})
    file(GLOB DetailFiles "${namevar}/*.c" "${namevar}/*.cpp" "${namevar}/*.cc")

    source_group(${sourcename} FILES ${DetailFiles})

    list(APPEND ${inoutvar} ${DetailFiles})  
    set(${inoutvar} ${${inoutvar}} PARENT_SCOPE)
endfunction()




function(collect_files output_var)
    # 
    set(file_list
        "file1.cpp"
        "file2.cpp"
        "file3.cpp"
    )
    # 
    set(${output_var} ${file_list} PARENT_SCOPE)
endfunction()

function(CollectHeaders target_dir header_files)
    file(GLOB CURRENT_HEADERS "${target_dir}/*.h" "${target_dir}/*.hpp")
	set(${header_files} ${CURRENT_HEADERS} PARENT_SCOPE)
endfunction()


function(ProjFolder projName folderName)
	if(MSVC)
		set_property(TARGET ${projName} PROPERTY FOLDER ${folderName})
	endif()
endfunction()

function(ProjInstallIncludDirs projName includDir)
	install(
 		DIRECTORY 
  		${includDir}
 	 	DESTINATION
  		"${projName}/${CMAKE_INSTALL_INCLUDEDIR}"
	 )
endfunction()

function(ProjInstallLibs projName)
if(NOT SYSTEM.iOS)
	install(TARGETS ${projName}
  		EXPORT ${projName}Targets
  	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	)
endif()
endfunction()

#the outer must set     
#set(HEADER_ALL_FILES)
macro(CollectHeaderMac basepath)
    file(GLOB COMMON_HEADER "${basepath}/*.h" "${basepath}/*.hpp")
    list(APPEND HEADER_ALL_FILES ${COMMON_HEADER})
    #message(STATUS "This is your headers: ${HEADERS}")
endmacro()


macro(CollectSrcMac basepath)
    file(GLOB COMMON_HEADER "${basepath}/*.c" "${basepath}/*.cpp" "${basepath}/*.cc")
    list(APPEND SRC_ALL_FILES ${COMMON_HEADER})
    #message(STATUS "This is your headers: ${HEADERS}")
endmacro()