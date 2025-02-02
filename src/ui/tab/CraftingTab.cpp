#include <iostream>

#include "game/Game.h"
#include "game/Inventory.h"
#include "recipe/CraftingRecipe.h"
#include "registry/Registries.h"
#include "ui/MainWindow.h"
#include "ui/gtk/EntryDialog.h"
#include "ui/gtk/NumericEntry.h"
#include "ui/gtk/Util.h"
#include "ui/tab/CraftingTab.h"
#include "ui/tab/InventoryTab.h"
#include "util/Util.h"

namespace Game3 {
	CraftingTab::CraftingTab(MainWindow &main_window): Tab(main_window.notebook), mainWindow(main_window) {
		scrolled.set_vexpand(true);
		scrolled.add_css_class("crafting-tab");
		scrolled.set_child(vbox);

		auto gmenu = Gio::Menu::create();
		gmenu->append("Craft _1", "crafting_popup.craft_one");
		gmenu->append("Craft _X", "crafting_popup.craft_x");
		gmenu->append("Craft _All", "crafting_popup.craft_all");
		popoverMenu.set_menu_model(gmenu);

		auto group = Gio::SimpleActionGroup::create();
		group->add_action("craft_one", [this] { craftOne(lastGame, lastRegistryID); });
		group->add_action("craft_x",   [] { std::cout << "x\n"; });
		group->add_action("craft_all", [this] { craftAll(lastGame, lastRegistryID); });

		mainWindow.insert_action_group("crafting_popup", group);
		popoverMenu.set_parent(mainWindow); // TODO: fix this silliness
	}

	void CraftingTab::update(const std::shared_ptr<Game> &) {}

	void CraftingTab::reset(const std::shared_ptr<Game> &game) {
		if (!game)
			return;

		lastGame = game;

		// Perhaps I ought to use a grid.
		removeChildren(vbox);
		widgets.clear();

		// size_t index = 0;
		auto inventory = game->player->inventory;
		for (const auto &recipe: game->registries.get<CraftingRecipeRegistry>().items) {
			if (game->player->stationTypes.contains(recipe->stationType) && recipe->canCraft(inventory)) {
				auto hbox = std::make_unique<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
				auto left_vbox = std::make_unique<Gtk::Box>(Gtk::Orientation::VERTICAL);
				auto right_vbox = std::make_unique<Gtk::Box>(Gtk::Orientation::VERTICAL);
				Glib::ustring output_label_text;
				for (ItemStack &output: recipe->output) {
					auto fixed = std::make_unique<Gtk::Fixed>();
					auto image = std::make_unique<Gtk::Image>(output.getImage(*game));
					auto label = std::make_unique<Gtk::Label>(std::to_string(output.count));
					if (!output_label_text.empty())
						output_label_text += " + ";
					if (output.count != 1)
						output_label_text += std::to_string(output.count) + ' ';
					output_label_text += output.item->name;
					Glib::ustring tooltip = output.item->name;
					if (output.count != 1)
						tooltip += " \u00d7 " + std::to_string(output.count);
					label->set_tooltip_text(tooltip);
					label->set_xalign(1.f);
					label->set_yalign(1.f);
					image->set_size_request(InventoryTab::TILE_SIZE - InventoryTab::TILE_MAGIC, InventoryTab::TILE_SIZE - InventoryTab::TILE_MAGIC);
					label->set_size_request(InventoryTab::TILE_SIZE - InventoryTab::TILE_MAGIC, InventoryTab::TILE_SIZE - InventoryTab::TILE_MAGIC);
					fixed->put(*image, 0, 0);
					fixed->put(*label, 0, 0);
					left_vbox->append(*fixed);
					widgets.push_back(std::move(fixed));
					widgets.push_back(std::move(image));
					widgets.push_back(std::move(label));
				}

				auto output_label = std::make_unique<Gtk::Label>(output_label_text);
				output_label->set_xalign(0.f);
				output_label->add_css_class("output-label");
				right_vbox->append(*output_label);

				for (const auto &input: recipe->input) {
					std::unique_ptr<Gtk::Label> label;
					if (input.is<ItemStack>()) {
						const auto &stack = input.get<ItemStack>();
						label = std::make_unique<Gtk::Label>((stack.count != 1? std::to_string(stack.count) + " \u00d7 " : "") + stack.item->name);
					} else {
						const auto &[attribute, count] = input.get<AttributeRequirement>();
						label = std::make_unique<Gtk::Label>((count != 1? std::to_string(count) + " \u00d7 " : "") + "any " + attribute.getPostPath());
					}
					label->set_xalign(0.f);
					label->add_css_class("input-label");
					right_vbox->append(*label);
					widgets.push_back(std::move(label));
				}

				right_vbox->add_css_class("right");
				hbox->add_css_class("recipe");
				hbox->append(*left_vbox);
				hbox->append(*right_vbox);
				vbox.append(*hbox);

				auto left_click = Gtk::GestureClick::create();
				left_click->set_button(1);
				left_click->signal_pressed().connect([this, game, hbox = hbox.get(), id = recipe->registryID](int n, double x, double y) {
					leftClick(game, hbox, id, n, x, y);
				});
				hbox->add_controller(left_click);

				auto right_click = Gtk::GestureClick::create();
				right_click->set_button(3);
				right_click->signal_pressed().connect([this, game, hbox = hbox.get(), id = recipe->registryID](int, double x, double y) {
					rightClick(game, hbox, id, x, y);
				});
				hbox->add_controller(right_click);

				widgets.push_back(std::move(output_label));
				widgets.push_back(std::move(left_vbox));
				widgets.push_back(std::move(right_vbox));
				widgets.push_back(std::move(hbox));
			}
		}
	}

	bool CraftingTab::craftOne(const std::shared_ptr<Game> &game, size_t registry_id) {
		auto recipe = game->registries.get<CraftingRecipeRegistry>()[registry_id];
		auto inventory = game->player->inventory;
		std::vector<ItemStack> leftovers;
		if (!recipe->craft(inventory, leftovers))
			return false;
		for (auto &leftover: leftovers)
			leftover.spawn(game->player->getRealm(), game->player->position);
		return true;
	}

	size_t CraftingTab::craftAll(const std::shared_ptr<Game> &game, size_t registry_id) {
		size_t out = 0;
		while (craftOne(game, registry_id))
			++out;
		return out;
	}

	void CraftingTab::leftClick(const std::shared_ptr<Game> &game, Gtk::Widget *, size_t registry_id, int n, double, double) {
		mainWindow.onBlur();
		if (n % 2 == 0)
			craftOne(game, registry_id);
	}

	void CraftingTab::rightClick(const std::shared_ptr<Game> &game, Gtk::Widget *widget, size_t registry_id, double x, double y) {
		mainWindow.onBlur();
		do {
			const auto allocation = widget->get_allocation();
			x += allocation.get_x();
			y += allocation.get_y();
			widget = widget->get_parent();
		} while (widget);

		popoverMenu.set_has_arrow(true);
		popoverMenu.set_pointing_to({int(x), int(y), 1, 1});
		lastGame = game;
		lastRegistryID = registry_id;
		popoverMenu.popup();
	}
}
