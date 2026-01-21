#pragma once

#include <tui++/SystemColors.h>

#include <string_view>

namespace tui::SystemColorKeys {
namespace detail {
struct SystemColorKey {
  const std::string_view key;
  const SystemColorIndex index;

  constexpr operator std::string_view const&() const {
    return this->key;
  }

  constexpr operator SystemColorIndex() const {
    return this->index;
  }
};
}

// @formatter:off
constexpr detail::SystemColorKey DESKTOP                  = { "desktop"              , SystemColorIndex::DESKTOP                 }; /* Color of the desktop background */
constexpr detail::SystemColorKey ACTIVE_CAPTION           = { "activeCaption"        , SystemColorIndex::ACTIVE_CAPTION          }; /* Color for captions (title bars) when they are active. */
constexpr detail::SystemColorKey ACTIVE_CAPTION_TEXT      = { "activeCaptionText"    , SystemColorIndex::ACTIVE_CAPTION_TEXT     }; /* Text color for text in captions (title bars). */
constexpr detail::SystemColorKey ACTIVE_CAPTION_BORDER    = { "activeCaptionBorder"  , SystemColorIndex::ACTIVE_CAPTION_BORDER   }; /* Border color for caption (title bar) window borders. */
constexpr detail::SystemColorKey INACTIVE_CAPTION         = { "inactiveCaption"      , SystemColorIndex::INACTIVE_CAPTION        }; /* Color for captions (title bars) when not active. */
constexpr detail::SystemColorKey INACTIVE_CAPTION_TEXT    = { "inactiveCaptionText"  , SystemColorIndex::INACTIVE_CAPTION_TEXT   }; /* Text color for text in inactive captions (title bars). */
constexpr detail::SystemColorKey INACTIVE_CAPTION_BORDER  = { "inactiveCaptionBorder", SystemColorIndex::INACTIVE_CAPTION_BORDER }; /* Border color for inactive caption (title bar) window borders. */
constexpr detail::SystemColorKey WINDOW                   = { "window"               , SystemColorIndex::WINDOW                  }; /* Background color for interior regions inside windows */
constexpr detail::SystemColorKey WINDOW_BORDER            = { "windowBorder"         , SystemColorIndex::WINDOW_BORDER           }; /* Border color for interior regions inside windows. */
constexpr detail::SystemColorKey WINDOW_TEXT              = { "windowText"           , SystemColorIndex::WINDOW_TEXT             }; /* Text color for interior regions inside windows. */
constexpr detail::SystemColorKey MENU                     = { "menu"                 , SystemColorIndex::MENU                    }; /* Background color for menus */
constexpr detail::SystemColorKey MENU_TEXT                = { "menuText"             , SystemColorIndex::MENU_TEXT               }; /* Text color for menus  */
constexpr detail::SystemColorKey TEXT                     = { "text"                 , SystemColorIndex::TEXT                    }; /* Text background color */
constexpr detail::SystemColorKey TEXT_TEXT                = { "textText"             , SystemColorIndex::TEXT_TEXT               }; /* Text foreground color */
constexpr detail::SystemColorKey TEXT_HIGHLIGHT           = { "textHighlight"        , SystemColorIndex::TEXT_HIGHLIGHT          }; /* Text background color when selected */
constexpr detail::SystemColorKey TEXT_HIGHLIGHT_TEXT      = { "textHighlightText"    , SystemColorIndex::TEXT_HIGHLIGHT_TEXT     }; /* Text color when selected */
constexpr detail::SystemColorKey TEXT_INACTIVE_TEXT       = { "textInactiveText"     , SystemColorIndex::TEXT_INACTIVE_TEXT      }; /* Text color when disabled */
constexpr detail::SystemColorKey CONTROL                  = { "control"              , SystemColorIndex::CONTROL                 }; /* Default color for controls (buttons, sliders, etc) */
constexpr detail::SystemColorKey CONTROL_TEXT             = { "controlText"          , SystemColorIndex::CONTROL_TEXT            }; /* Default color for text in controls */
constexpr detail::SystemColorKey CONTROL_HIGHLIGHT        = { "controlHighlight"     , SystemColorIndex::CONTROL_HIGHLIGHT       }; /* Specular highlight (opposite of the shadow) */
constexpr detail::SystemColorKey CONTROL_LT_HIGHLIGHT     = { "controlLtHighlight"   , SystemColorIndex::CONTROL_LT_HIGHLIGHT    }; /* Highlight color for controls */
constexpr detail::SystemColorKey CONTROL_SHADOW           = { "controlShadow"        , SystemColorIndex::CONTROL_SHADOW          }; /* Shadow color for controls */
constexpr detail::SystemColorKey CONTROL_DK_SHADOW        = { "controlDkShadow"      , SystemColorIndex::CONTROL_DK_SHADOW       }; /* Dark shadow color for controls */
constexpr detail::SystemColorKey SCROLLBAR                = { "scrollbar"            , SystemColorIndex::SCROLLBAR               }; /* Scrollbar background (usually the "track") */
constexpr detail::SystemColorKey INFO                     = { "info"                 , SystemColorIndex::INFO                    }; /* Background color for tooltips or spot help */
constexpr detail::SystemColorKey INFO_TEXT                = { "infoText"             , SystemColorIndex::INFO_TEXT               }; /* Text color for tooltips or spot help */

constexpr detail::SystemColorKey SYSTEM_COLOR_KEYS[] = {
  DESKTOP                , //
  ACTIVE_CAPTION         , //
  ACTIVE_CAPTION_TEXT    , //
  ACTIVE_CAPTION_BORDER  , //
  INACTIVE_CAPTION       , //
  INACTIVE_CAPTION_TEXT  , //
  INACTIVE_CAPTION_BORDER, //
  WINDOW                 , //
  WINDOW_BORDER          , //
  WINDOW_TEXT            , //
  MENU                   , //
  MENU_TEXT              , //
  TEXT                   , //
  TEXT_TEXT              , //
  TEXT_HIGHLIGHT         , //
  TEXT_HIGHLIGHT_TEXT    , //
  TEXT_INACTIVE_TEXT     , //
  CONTROL                , //
  CONTROL_TEXT           , //
  CONTROL_HIGHLIGHT      , //
  CONTROL_LT_HIGHLIGHT   , //
  CONTROL_SHADOW         , //
  CONTROL_DK_SHADOW      , //
  SCROLLBAR              , //
  INFO                   , //
  INFO_TEXT              , //
};

// @formatter:on
}
