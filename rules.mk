# $(warning in $(src))

CPPFLAGS = -Iinclude

include $(wildcard */rules.mk)
