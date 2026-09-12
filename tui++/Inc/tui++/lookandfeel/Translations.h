#pragma once

#include <string>
#include <string_view>

namespace tui {

class Component;

namespace laf {

/**
 * The text the look-and-feel draws for a widget: the widget's own text looked
 * up in the translation bundles (see Messages).
 *
 * The lookup happens here, where the text is measured and painted, so the
 * translation is orthogonal to the components: no component carries
 * translation state or a translation API, and the widget's own text (what a
 * program sets and reads back through get_text) is never changed. A widget
 * whose text is data rather than a phrase opts out through the ordinary client
 * property mechanism:
 *
 *   button->set_client_property(Messages::TRANSLATABLE_PROPERTY, false);
 *
 * Messages::set_locale (and any bundle change) drops the layouts of the live
 * windows and repaints them, so a widget on screen follows with its text and
 * the size it measures.
 */
std::string displayed_text(Component const &component, std::string_view text);

}

}
