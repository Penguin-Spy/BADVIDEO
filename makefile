# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        ?= BADVIDEO
COMPRESSED  ?= NO
ICON        ?= icon.png
DESCRIPTION ?= "Bad Apple for the TI 84+ CE (but its black and white anyways, so yeah)"

# ----------------------------
# Other Options (Advanced)
# ----------------------------

#EXTRA_CFLAGS        ?=
#USE_FLASH_FUNCTIONS ?= YES|NO
#OUTPUT_MAP          ?= YES|NO
#ARCHIVED            ?= YES|NO
#OPT_MODE            ?= -optsize|-optspeed
#SRCDIR              ?= src
#OBJDIR              ?= obj
#BINDIR              ?= bin
#GFXDIR              ?= src/gfx
#V                   ?= 1

include $(CEDEV)/include/.makefile


VIDEO                := $(basename $(notdir $(wildcard src/video/*.png)))


VIDEOBINDIR          := $(BINDIR)
VIDEOSRCDIR          := $(SRCDIR)/video
VIDEO8XV             := $(VIDEO).8xv


$(BINDIR)/%.8xv: $(BINDIR)/%.bin
	$(Q)echo $^ $@
	$(CONVBIN) -n $(basename $(notdir $@)) -j bin -k 8xv -i $^ -o $@

$(BINDIR)/%.bin: $(VIDEOSRCDIR)/%.mp4
	$(Q)echo Converting $^ to $@
	py convertFrame.py $^ $@

test:
	$(Q)echo $(VIDEO)
