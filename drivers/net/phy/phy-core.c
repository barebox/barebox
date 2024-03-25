// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <linux/phy.h>

/**
 * phy_modify - Convenience function for modifying a given PHY register
 * @phydev: the phy_device struct
 * @regnum: register number to write
 * @mask: bit mask of bits to clear
 * @set: new value of bits set in mask to write to @regnum
 *
 */
int phy_modify(struct phy_device *phydev, u32 regnum, u16 mask, u16 set)
{
	int ret;

	ret = phy_read(phydev, regnum);
	if (ret < 0)
		return ret;

	ret = phy_write(phydev, regnum, (ret & ~mask) | set);

	return ret < 0 ? ret : 0;
}

static int phy_read_page(struct phy_device *phydev)
{
	struct phy_driver *phydrv = to_phy_driver(phydev->dev.driver);

	if (!phydrv->read_page) {
		dev_warn_once(&phydev->dev, "read_page callback not available, PHY driver not loaded?\n");
		return -EOPNOTSUPP;
	}

	return phydrv->read_page(phydev);
}

static int phy_write_page(struct phy_device *phydev, int page)
{
	struct phy_driver *phydrv = to_phy_driver(phydev->dev.driver);

	if (!phydrv->write_page) {
		dev_warn_once(&phydev->dev, "write_page callback not available, PHY driver not loaded?\n");
		return -EOPNOTSUPP;
	}

	return phydrv->write_page(phydev, page);
}

/**
 * phy_save_page() - take the bus lock and save the current page
 * @phydev: a pointer to a &struct phy_device
 *
 * Take the MDIO bus lock, and return the current page number. On error,
 * returns a negative errno. phy_restore_page() must always be called
 * after this, irrespective of success or failure of this call.
 */
int phy_save_page(struct phy_device *phydev)
{
	return phy_read_page(phydev);
}

/**
 * phy_select_page() - take the bus lock, save the current page, and set a page
 * @phydev: a pointer to a &struct phy_device
 * @page: desired page
 *
 * Take the MDIO bus lock to protect against concurrent access, save the
 * current PHY page, and set the current page.  On error, returns a
 * negative errno, otherwise returns the previous page number.
 * phy_restore_page() must always be called after this, irrespective
 * of success or failure of this call.
 */
int phy_select_page(struct phy_device *phydev, int page)
{
	int ret, oldpage;

	oldpage = ret = phy_save_page(phydev);
	if (ret < 0)
		return ret;

	if (oldpage != page) {
		ret = phy_write_page(phydev, page);
		if (ret < 0)
			return ret;
	}

	return oldpage;
}

/**
 * phy_restore_page() - restore the page register and release the bus lock
 * @phydev: a pointer to a &struct phy_device
 * @oldpage: the old page, return value from phy_save_page() or phy_select_page()
 * @ret: operation's return code
 *
 * Release the MDIO bus lock, restoring @oldpage if it is a valid page.
 * This function propagates the earliest error code from the group of
 * operations.
 *
 * Returns:
 *   @oldpage if it was a negative value, otherwise
 *   @ret if it was a negative errno value, otherwise
 *   phy_write_page()'s negative value if it were in error, otherwise
 *   @ret.
 */
int phy_restore_page(struct phy_device *phydev, int oldpage, int ret)
{
	int r;

	if (oldpage >= 0) {
		r = phy_write_page(phydev, oldpage);

		/* Propagate the operation return code if the page write
		 * was successful.
		 */
		if (ret >= 0 && r < 0)
			ret = r;
	} else {
		/* Propagate the phy page selection error code */
		ret = oldpage;
	}

	return ret;
}

/**
 * phy_read_paged() - Convenience function for reading a paged register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number
 *
 * Same rules as for phy_read().
 */
int phy_read_paged(struct phy_device *phydev, int page, u32 regnum)
{
	int ret = 0, oldpage;

	oldpage = phy_select_page(phydev, page);
	if (oldpage >= 0)
		ret = phy_read(phydev, regnum);

	return phy_restore_page(phydev, oldpage, ret);
}

/**
 * phy_write_paged() - Convenience function for writing a paged register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number
 * @val: value to write
 *
 * Same rules as for phy_write().
 */
int phy_write_paged(struct phy_device *phydev, int page, u32 regnum, u16 val)
{
	int ret = 0, oldpage;

	oldpage = phy_select_page(phydev, page);
	if (oldpage >= 0)
		ret = phy_write(phydev, regnum, val);

	return phy_restore_page(phydev, oldpage, ret);
}

/**
 * phy_modify_paged() - Convenience function for modifying a paged register
 * @phydev: a pointer to a &struct phy_device
 * @page: the page for the phy
 * @regnum: register number
 * @mask: bit mask of bits to clear
 * @set: bit mask of bits to set
 *
 * Same rules as for phy_read() and phy_write().
 */
int phy_modify_paged(struct phy_device *phydev, int page, u32 regnum,
		     u16 mask, u16 set)
{
	int ret = 0, oldpage;

	oldpage = phy_select_page(phydev, page);
	if (oldpage >= 0)
		ret = phy_modify(phydev, regnum, mask, set);

	return phy_restore_page(phydev, oldpage, ret);
}
