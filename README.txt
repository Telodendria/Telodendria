            .=                       -=-               
          :.:+                     .=:.                
         .=+-==.                  :.                   
           .+-                   =.                    
           .+                   :+.                    
            ==.                 -+:                    
             =++==--::           =+.                   
               .:::--=+=:        :+=                   
                       :==.      -=:                   
                         ===----=-.           ... :+.  
                       :==+=======:        .-+-::-+-=+=
                      .==*%#=======       :+-      ..  
                 .:--=-===+=========-.   :+:           
              .=++=::..:============-+=-=-             
:+=:        :=+-:      .-=========-.  .                
 =+++:  .:=+-:      .:--. .--:==:                      
   ::---:..       :=+:        ==                       
                  ++.        .+-                       
                  =+         .+-     ...:              
                  +-          -+-:-+=::+:              
        :=-....:-=:            .--:    =-              
     -++=:.:::..                                       

=======================================================
|_   _|__| | ___   __| | ___ _ __   __| |_ __(_) __ _ 
  | |/ _ \ |/ _ \ / _` |/ _ \ '_ \ / _` | '__| |/ _` |
  | |  __/ | (_) | (_| |  __/ | | | (_| | |  | | (_| |
  |_|\___|_|\___/ \__,_|\___|_| |_|\__,_|_|  |_|\__,_|
=======================================================
Copyright (C) 2023 Jordan Bancino <@jordan:bancino.net>

This is the source code for Telodendria, a Matrix homeserver written
in C. All of the documentation is available as man pages in the
man/ directory, or online at https://telodendria.io

If information is missing from the documentation, please feel free
to reach out to #telodendria-general:bancino.net on Matrix.

This file documents the directory structure of the source code
repository.

Telodendria/
	contrib/ - Supplemental files, such as example configs.
	man/ - The official documentation as man pages.
    proposals/ - Proposals for new features or fixes, as man pages.
	site/ - The official website.
	src/ - The C source code for Telodendria.
		include/ - Header files for the source code.
		Routes/ - Where Matrix API endpoints are implemented.
        Static/ - Endpoints that just generate static HTML pages.
	tests/ - Unit and integration tests will eventually go here.
	tools/ - Development environment and tools.

To cut a new release for Telodendria, perform the following
steps. This is just a reference for me so I don't mess it up.

	- Update tools/bin/td to declare the next version number.
	- Make sure man/man7/telodendria-changelog.7 is up to date.
	  with the latest information.
	- Commit all changes.
	- Run the release recipe: td release
	- Deploy the site: td site

