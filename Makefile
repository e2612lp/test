include $(TOPDIR)/rules.mk

PKG_NAME:=dtu_test
PKG_VERSION:=1.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

define Package/dtu_test
	SECTION:=base
	CATEGORY:=Utilities
	TITLE:= dtu test uart transmit	
	DEPENDS:=+libc
	DEPENDS:=+libpthread
endef
    
define Package/dtu_test/description
	If you can't figure out what this program does, you're probably  
	brain-dead and need immediate medical attention.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./inc/* $(PKG_BUILD_DIR)/
endef

define Package/dtu_test/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dtu_test $(1)/bin/
endef

$(eval $(call BuildPackage,dtu_test))
