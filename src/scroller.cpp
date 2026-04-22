#include "zep/scroller.h"
#include "zep/buffer.h"
#include "zep/display.h"
#include "zep/editor.h"
#include "zep/theme.h"

#include "zep/mcommon/logger.h"

// A scrollbar that is manually drawn and implemented.  This means it is independent of the backend and can be drawn the
// same in Qt and ImGui
namespace Zep
{

void Scroller::SetErrorMarkers(const std::vector<std::shared_ptr<RangeMarker>>& errors, float totalHeight, float viewStart, ZepBuffer* pBuffer)
{
    m_totalTextHeight = totalHeight;
    m_viewStart = viewStart;
    m_errorPositions.clear();

    if (totalHeight <= 0.0f || m_mainRegion->rect.Height() <= 0.0f || pBuffer == nullptr)
        return;

    auto bufferSize = pBuffer->GetWorkingBuffer().size();
    if (bufferSize == 0)
        return;

    for (const auto& marker : errors)
    {
        auto range = marker->GetRange();
        float bytePos = (float)range.first;
        float normalizedPos = bytePos / (float)bufferSize;
        float yPos = normalizedPos * totalHeight;
        m_errorPositions.push_back({ yPos, marker });
    }
}

void Scroller::ClearErrorMarkers()
{
    m_errorPositions.clear();
    m_totalTextHeight = 0.0f;
    m_viewStart = 0.0f;
}

Scroller::Scroller(ZepEditor& editor, Region& parent)
    : ZepComponent(editor)
{
    m_region = std::make_shared<Region>();
    m_topButtonRegion = std::make_shared<Region>();
    m_bottomButtonRegion = std::make_shared<Region>();
    m_mainRegion = std::make_shared<Region>();

    m_region->flags = RegionFlags::Expanding;
    m_topButtonRegion->flags = RegionFlags::Fixed;
    m_bottomButtonRegion->flags = RegionFlags::Fixed;
    m_mainRegion->flags = RegionFlags::Expanding;

    m_region->layoutType = RegionLayoutType::VBox;

    const float scrollButtonMargin = 3.0f * editor.GetDisplay().GetPixelScale().x;
    m_topButtonRegion->padding = NVec2f(scrollButtonMargin, scrollButtonMargin);
    m_bottomButtonRegion->padding = NVec2f(scrollButtonMargin, scrollButtonMargin);
    m_mainRegion->padding = NVec2f(scrollButtonMargin, 0.0f);

    const float scrollButtonSize = DefaultTextSize * editor.GetDisplay().GetPixelScale().x;
    m_topButtonRegion->fixed_size = NVec2f(0.0f, scrollButtonSize);
    m_bottomButtonRegion->fixed_size = NVec2f(0.0f, scrollButtonSize);

    m_region->children.push_back(m_topButtonRegion);
    m_region->children.push_back(m_mainRegion);
    m_region->children.push_back(m_bottomButtonRegion);

    parent.children.push_back(m_region);
}

void Scroller::CheckState()
{
    if (m_scrollState == ScrollState::None)
    {
        return;
    }

    if (timer_get_elapsed_seconds(m_start_delay_timer) < 0.5f)
    {
        return;
    }

    switch (m_scrollState)
    {
    default:
    case ScrollState::None:
        break;
    case ScrollState::ScrollUp:
        ClickUp();
        break;
    case ScrollState::ScrollDown:
        ClickDown();
        break;
    case ScrollState::PageUp:
        PageUp();
        break;
    case ScrollState::PageDown:
        PageDown();
        break;
    }

    GetEditor().RequestRefresh();
}

void Scroller::ClickUp()
{
    vScrollPosition -= vScrollLinePercent;
    vScrollPosition = std::max(0.0f, vScrollPosition);
    GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::ComponentChanged, this));
    m_scrollState = ScrollState::ScrollUp;
}

void Scroller::ClickDown()
{
    vScrollPosition += vScrollLinePercent;
    vScrollPosition = std::min(1.0f - vScrollVisiblePercent, vScrollPosition);
    GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::ComponentChanged, this));
    m_scrollState = ScrollState::ScrollDown;
}

void Scroller::PageUp()
{
    vScrollPosition -= vScrollPagePercent;
    vScrollPosition = std::max(0.0f, vScrollPosition);
    GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::ComponentChanged, this));
    m_scrollState = ScrollState::PageUp;
}

void Scroller::PageDown()
{
    vScrollPosition += vScrollPagePercent;
    vScrollPosition = std::min(1.0f - vScrollVisiblePercent, vScrollPosition);
    GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::ComponentChanged, this));
    m_scrollState = ScrollState::PageDown;
}

void Scroller::DoMove(NVec2f pos)
{
    if (m_scrollState == ScrollState::Drag)
    {
        float dist = pos.y - m_mouseDownPos.y;

        float totalMove = m_mainRegion->rect.Height() - ThumbSize();
        float percentPerPixel = (1.0f - vScrollVisiblePercent) / totalMove;
        vScrollPosition = m_mouseDownPercent + (percentPerPixel * dist);
        vScrollPosition = std::min(1.0f - vScrollVisiblePercent, vScrollPosition);
        vScrollPosition = std::max(0.0f, vScrollPosition);
        GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::ComponentChanged, this));
    }
}

float Scroller::ThumbSize() const
{
    return std::max(10.0f, m_mainRegion->rect.Height() * vScrollVisiblePercent);
}

float Scroller::ThumbExtra() const
{
    return std::max(0.0f, ThumbSize() - (m_mainRegion->rect.Height() * vScrollVisiblePercent));
}

NRectf Scroller::ThumbRect() const
{
    auto thumbSize = ThumbSize();
    return NRectf(NVec2f(m_mainRegion->rect.topLeftPx.x, m_mainRegion->rect.topLeftPx.y + m_mainRegion->rect.Height() * vScrollPosition), NVec2f(m_mainRegion->rect.bottomRightPx.x, m_mainRegion->rect.topLeftPx.y + m_mainRegion->rect.Height() * vScrollPosition + thumbSize));
}

void Scroller::Notify(std::shared_ptr<ZepMessage> message)
{
    switch (message->messageId)
    {
    case Msg::Tick:
    {
        CheckState();
    }
    break;

    case Msg::MouseDown:
        if (message->button == ZepMouseButton::Left)
        {
            if (m_bottomButtonRegion->rect.Contains(message->pos))
            {
                ClickDown();
                timer_start(m_start_delay_timer);
                message->handled = true;
            }
            else if (m_topButtonRegion->rect.Contains(message->pos))
            {
                ClickUp();
                timer_start(m_start_delay_timer);
                message->handled = true;
            }
            else if (m_mainRegion->rect.Contains(message->pos))
            {
                // Check if clicked on an error indicator
                if (!m_errorPositions.empty() && m_totalTextHeight > 0.0f)
                {
                    float viewportTop = m_viewStart;
                    float viewportBottom = m_viewStart + (m_mainRegion->rect.Height() * vScrollVisiblePercent * m_totalTextHeight / m_mainRegion->rect.Height());

                    for (const auto& errorPos : m_errorPositions)
                    {
                        float yPos = errorPos.first;
                        bool isAbove = yPos < viewportTop;
                        bool isBelow = yPos > viewportBottom;

                        if (isAbove || isBelow)
                        {
                            float indicatorY;
                            if (isAbove)
                            {
                                indicatorY = m_mainRegion->rect.topLeftPx.y + m_mainRegion->rect.Height() * 0.1f;
                            }
                            else
                            {
                                indicatorY = m_mainRegion->rect.bottomRightPx.y - m_mainRegion->rect.Height() * 0.1f - 8.0f;
                            }

                            // Check if click is near the indicator
                            if (std::abs(message->pos.y - indicatorY) < 15.0f && std::abs(message->pos.x - m_mainRegion->rect.Center().x) < 15.0f)
                            {
                                // Broadcast message to scroll to the error
                                auto pMsg = std::make_shared<ZepMessage>(Msg::GoToMarker, message->pos);
                                pMsg->spMarker = errorPos.second;
                                GetEditor().Broadcast(pMsg);
                                message->handled = true;
                                return;
                            }
                        }
                    }
                }

                auto thumbRect = ThumbRect();
                if (thumbRect.Contains(message->pos))
                {
                    m_mouseDownPos = message->pos;
                    m_mouseDownPercent = vScrollPosition;
                    m_scrollState = ScrollState::Drag;
                    message->handled = true;
                }
                else if (message->pos.y > thumbRect.BottomLeft().y)
                {
                    PageDown();
                    timer_start(m_start_delay_timer);
                    message->handled = true;
                }
                else if (message->pos.y < thumbRect.TopRight().y)
                {
                    PageUp();
                    timer_start(m_start_delay_timer);
                    message->handled = true;
                }
            }
        }
        break;
    case Msg::MouseUp:
    {
        m_scrollState = ScrollState::None;
    }
    break;
    case Msg::MouseMove:
        DoMove(message->pos);
        break;
    default:
        break;
    }
}

void Scroller::Display(ZepTheme& theme)
{
    auto& display = GetEditor().GetDisplay();

    display.SetClipRect(m_region->rect);

    auto mousePos = GetEditor().GetMousePos();
    auto activeColor = theme.GetColor(ThemeColor::WidgetActive);
    auto inactiveColor = theme.GetColor(ThemeColor::WidgetInactive);
    auto errorColor = theme.GetColor(ThemeColor::Error);

    // Scroller background
    display.DrawRectFilled(m_region->rect, theme.GetColor(ThemeColor::WidgetBackground));

    bool onTop = m_topButtonRegion->rect.Contains(mousePos) && m_scrollState != ScrollState::Drag;
    bool onBottom = m_bottomButtonRegion->rect.Contains(mousePos) && m_scrollState != ScrollState::Drag;

    if (m_scrollState == ScrollState::ScrollUp)
    {
        onTop = true;
    }
    if (m_scrollState == ScrollState::ScrollDown)
    {
        onBottom = true;
    }

    display.DrawRectFilled(m_topButtonRegion->rect, onTop ? activeColor : inactiveColor);
    display.DrawRectFilled(m_bottomButtonRegion->rect, onBottom ? activeColor : inactiveColor);

    auto thumbRect = ThumbRect();

    // Thumb
    display.DrawRectFilled(thumbRect, thumbRect.Contains(mousePos) || m_scrollState == ScrollState::Drag ? activeColor : inactiveColor);

    // Draw error indicators outside the viewport
    if (!m_errorPositions.empty() && m_totalTextHeight > 0.0f && m_mainRegion->rect.Height() > 0.0f)
    {
        float viewportTop = m_viewStart;
        float viewportBottom = m_viewStart + (m_mainRegion->rect.Height() * vScrollVisiblePercent * m_totalTextHeight / m_mainRegion->rect.Height());

        for (const auto& errorPos : m_errorPositions)
        {
            float yPos = errorPos.first;
            bool isAbove = yPos < viewportTop;
            bool isBelow = yPos > viewportBottom;

            if (isAbove || isBelow)
            {
                float indicatorY;
                if (isAbove)
                {
                    indicatorY = m_mainRegion->rect.topLeftPx.y + m_mainRegion->rect.Height() * 0.1f;
                }
                else
                {
                    indicatorY = m_mainRegion->rect.bottomRightPx.y - m_mainRegion->rect.Height() * 0.1f - 8.0f;
                }

                // Draw arrow indicator
                NVec2f arrowCenter(m_mainRegion->rect.Center().x, indicatorY);
                float arrowSize = 6.0f;

                // Triangle pointing up or down
                std::vector<NVec2f> triangle;
                if (isAbove)
                {
                    triangle = {
                        NVec2f(arrowCenter.x, arrowCenter.y - arrowSize),
                        NVec2f(arrowCenter.x - arrowSize, arrowCenter.y + arrowSize),
                        NVec2f(arrowCenter.x + arrowSize, arrowCenter.y + arrowSize)
                    };
                }
                else
                {
                    triangle = {
                        NVec2f(arrowCenter.x, arrowCenter.y + arrowSize),
                        NVec2f(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize),
                        NVec2f(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize)
                    };
                }

                display.DrawLine(triangle[0], triangle[1], errorColor, 2.0f);
                display.DrawLine(triangle[1], triangle[2], errorColor, 2.0f);
                display.DrawLine(triangle[2], triangle[0], errorColor, 2.0f);
            }
        }
    }
}

}; // namespace Zep