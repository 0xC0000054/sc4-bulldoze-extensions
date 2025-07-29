#pragma once
#include "cIGZUnknown.h"

class cIGZCursor;
class cIGZWin;

class cISC4ViewInputControl : public cIGZUnknown
{
public:

	virtual bool Init() = 0;
	virtual bool Shutdown() = 0;

	virtual uint32_t GetID() = 0;
	virtual bool SetID(uint32_t id) = 0;

	virtual cIGZCursor* GetCursor() = 0;
	virtual bool SetCursor(cIGZCursor* cursor) = 0;
	virtual bool SetCursor(uint32_t cursorID) = 0;

	virtual bool SetWindow(cIGZWin* pWin) = 0;

	virtual bool IsSelfScrollingView() = 0;
	virtual bool ShouldStack() = 0;

	virtual bool OnCharacter(char value) = 0;
	virtual bool OnKeyDown(int32_t virtualKeyCode, uint32_t modifiers) = 0;
	virtual bool OnKeyUp(int32_t virtualKeyCode, uint32_t modifiers) = 0;
	virtual bool OnMouseDownL(int32_t unknown1, uint32_t unknown2) = 0;
	virtual bool OnMouseDownR(int32_t unknown1, uint32_t unknown2) = 0;
	virtual bool OnMouseUpL(int32_t unknown1, uint32_t unknown2) = 0;
	virtual bool OnMouseUpR(int32_t unknown1, uint32_t unknown2) = 0;
	virtual bool OnMouseMove(int32_t x, int32_t z, uint32_t modifiers) = 0;
	/// <summary>
	/// Called when the mouse wheel is scrolled over the view
	/// </summary>
	/// <param name="x">Mouse X coordinate in view space</param>
	/// <param name="z">Mouse Z coordinate in view space</param>
	/// <param name="modifiers">Modifier key flags (Shift=0x1, Ctrl=0x2, Alt=0x4)</param>
	/// <param name="wheelDelta">Wheel scroll delta (positive=up/away, negative=down/toward)</param>
	/// <returns>true if the event was handled, false to allow default processing</returns>
	virtual bool OnMouseWheel(int32_t x, int32_t z, uint32_t modifiers, int32_t wheelDelta) = 0;
	virtual bool OnMouseExit() = 0;

	virtual void Activate() = 0;
	virtual void Deactivate() = 0;
	virtual bool AmCapturing() = 0;
};
