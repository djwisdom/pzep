#pragma once

#include "editor.h"
#include "splits.h"
#include "zep/mcommon/animation/timer.h"

namespace Zep
{
class ZepTheme;
class ZepEditor;
class RangeMarker;

class Scroller : public ZepComponent
{
public:
    Scroller(ZepEditor& editor, Region& parent);

    virtual void Display(ZepTheme& theme);
    virtual void Notify(std::shared_ptr<ZepMessage> message) override;

    void SetErrorMarkers(const std::vector<std::shared_ptr<RangeMarker>>& errors, float totalHeight, float viewStart, ZepBuffer* pBuffer);
    void ClearErrorMarkers();

    float vScrollVisiblePercent = 1.0f;
    float vScrollPosition = 0.0f;
    float vScrollLinePercent = 0.0f;
    float vScrollPagePercent = 0.0f;
    bool vertical = true;

private:
    void CheckState();
    void ClickUp();
    void ClickDown();
    void PageUp();
    void PageDown();
    void DoMove(NVec2f pos);

    float ThumbSize() const;
    float ThumbExtra() const;
    NRectf ThumbRect() const;

private:
    std::shared_ptr<Region> m_region;
    std::shared_ptr<Region> m_topButtonRegion;
    std::shared_ptr<Region> m_bottomButtonRegion;
    std::shared_ptr<Region> m_mainRegion;
    timer m_start_delay_timer;
    timer m_reclick_timer;
    enum class ScrollState
    {
        None,
        ScrollDown,
        ScrollUp,
        PageUp,
        PageDown,
        Drag
    };
    ScrollState m_scrollState = ScrollState::None;
    NVec2f m_mouseDownPos;
    float m_mouseDownPercent;

    std::vector<std::pair<float, std::shared_ptr<RangeMarker>>> m_errorPositions;
    float m_totalTextHeight = 0.0f;
    float m_viewStart = 0.0f;
};

}; // namespace Zep
