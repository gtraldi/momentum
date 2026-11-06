import unreal

MAIN_WIDGET_CLASS_PATH = "/Script/SurfGame.MainMenuWidget"
PAUSE_WIDGET_CLASS_PATH = "/Script/SurfGame.PauseMenuWidget"
WBP_PACKAGE_PATH = "/Game/MainMenu"
WBP_MAIN_NAME = "WBP_MainMenu"
WBP_PAUSE_NAME = "WBP_PauseMenu"


def ensure_directory(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_or_update_wbp(asset_name: str, parent_class_path: str):
    """Cria ou atualiza um Widget Blueprint em /Game/MainMenu derivado da classe C++ informada."""
    ensure_directory(WBP_PACKAGE_PATH)
    full_wbp_path = f"{WBP_PACKAGE_PATH}/{asset_name}"
    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(f"[MainMenu] Classe C++ {parent_class_path} nao encontrada!")

    if unreal.EditorAssetLibrary.does_asset_exist(full_wbp_path):
        unreal.EditorAssetLibrary.delete_asset(full_wbp_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    wbp_asset = None

    try:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        wbp_asset = asset_tools.create_asset(asset_name, WBP_PACKAGE_PATH, unreal.Blueprint, factory)
    except Exception as exc:
        unreal.log_warning(f"[MainMenu] Aviso ao criar {asset_name} via BlueprintFactory: {exc}")

    if wbp_asset:
        unreal.EditorAssetLibrary.save_loaded_asset(wbp_asset)
        unreal.log(f"[MainMenu] Widget Blueprint {full_wbp_path} criado e salvo com sucesso.")


def main():
    create_or_update_wbp(WBP_MAIN_NAME, MAIN_WIDGET_CLASS_PATH)
    create_or_update_wbp(WBP_PAUSE_NAME, PAUSE_WIDGET_CLASS_PATH)
    unreal.log("[MainMenu] Configuracao do WBP_MainMenu e WBP_PauseMenu concluida com sucesso!")


if __name__ == "__main__":
    main()
