# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        ?= BADVIDEO
COMPRESSED  ?= NO
ICON        ?= src/icon.png
DESCRIPTION ?= "'Bad' 2-tone video player for the TI-84+ CE"

# ----------------------------
# Other Options (Advanced)
# ----------------------------

#EXTRA_CFLAGS        ?=
#USE_FLASH_FUNCTIONS ?= YES|NO
#OUTPUT_MAP          ?= YES|NO
ARCHIVED             ?= YES
#OPT_MODE            ?= -optsize|-optspeed
#SRCDIR              ?= src
#OBJDIR              ?= obj
#BINDIR              ?= bin
#GFXDIR              ?= src/gfx
#V                   ?= 1

include $(CEDEV)/include/.makefile

# yes this has to be below the include; gosh, make is sometimes so finicky
VIDEOBINDIR          := $(BINDIR)/video
VIDEOSRCDIR          := $(SRCDIR)/video

# target used by convertFrame.py for cross-platform convbin
$(VIDEOBINDIR)/%.8xv: $(VIDEOBINDIR)/%.bin
	$(Q)echo $^ $@
	$(Q)$(CONVBIN) -n $(basename $(notdir $@)) -j bin -k 8xv -i $^ -o $@ -r

# target used to create the various .8xv files from the .mp4 (.llv is a "phony file type" used to make this target different from the above one)
$(VIDEOBINDIR)/%.llv: $(VIDEOSRCDIR)/%.mp4
	$(Q)echo Converting $^ to $@
	$(Q)py tools/convertFrame.py $^ $(VIDEOBINDIR) $(basename $(notdir $@)) $(lastword $(filter-out $@, $(MAKECMDGOALS)))

# QoL target to just specify the name of a video & clean then convert it
%: cleanvideo $(VIDEOBINDIR)/%.llv
	$(Q)echo Created video $@

# Cleans all intermediate & resulting video binaries
cleanvideo:
	$(Q)$(RM) "bin/video/"
	$(Q)$(RM) temp.bin.zx7
	$(Q)$(RM) temp.bin

# gonna be honest, i forgot what .INTERMEDIATE means but i'll assume it says something like "hey don't delete my files while i'm in the middle of using them"
.INTERMEDIATE: temp.bin temp.bin.zx7

# tell make to not delete our outputs just because they have a different name than our supposed target
.PRECIOUS: $(VIDEOBINDIR)/%.bin
.PRECIOUS: $(VIDEOBINDIR)/%.8xv

# cleanvideo is actually fake news, not real, government propoganda, secret survelance drones used to track if you've learned the truth that the earth is flat
.PHONY: cleanvideo