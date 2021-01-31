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

VIDEOBINDIR          := $(BINDIR)/video
VIDEOSRCDIR          := $(SRCDIR)/video

# makes a group from a video (video → *.bin → *.8vx → NAME.8xg)
# doesn't work because groups must fit in 64k
#$(BINDIR)/%.8xg: $(VIDEOSRCDIR)/%.mp4
#	$(Q)echo Converting $^ to $@
#	py convertFrame.py $^ $(OBJDIR) $(basename $(notdir $@))
#	$(CONVBIN) -n $(basename $(notdir $@)) -j bin -k 8xg-auto-extract -o $@ -r $(addprefix -i , $(wildcard obj/*.8xv))


$(VIDEOBINDIR)/%.8xv: $(VIDEOBINDIR)/%.bin
	$(Q)echo $^ $@
	$(Q)$(CONVBIN) -n $(basename $(notdir $@)) -j bin -k 8xv -i $^ -o $@ -r

$(VIDEOBINDIR)/%.8xv: $(VIDEOSRCDIR)/%.mp4
	$(Q)echo Converting $^ to $@
	$(Q)py convertFrame.py $^ $(VIDEOBINDIR) $(basename $(notdir $@))


%: $(VIDEOBINDIR)/%.8xv
	$(Q)echo Created video $@

cleanvideo:
	$(Q)$(RM) $(VIDEOBINDIR)/*.bin
	$(Q)$(RM) $(VIDEOBINDIR)/*.8xv

.PRECIOUS: $(VIDEOBINDIR)/%.bin
.PRECIOUS: $(VIDEOBINDIR)/%.8xv

.PHONY: rmvideo