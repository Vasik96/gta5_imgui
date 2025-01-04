#pragma once


namespace globals {

    


	extern bool windowVisible;
	extern bool running;

    extern bool overlayToggled;

    

}

enum VisibilityStatus {
    VISIBLE,
    OVERLAY,
    HIDDEN
};


extern VisibilityStatus visibilityStatus;