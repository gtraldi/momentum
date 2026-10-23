import unreal

MAP_PACKAGE_PATH = "/Game/MainMenu/Maps/L_MainMenu"
EXPECTED_GAMEMODE_NAME = "MainMenuGameMode"


def main():
    unreal.log(f"[MainMenuVerify] Carregando {MAP_PACKAGE_PATH} para validacao...")

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE_PATH):
        raise RuntimeError(f"[MainMenuVerify] ERRO: O asset {MAP_PACKAGE_PATH} nao existe!")

    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PACKAGE_PATH)
    if not world:
        raise RuntimeError(f"[MainMenuVerify] ERRO: Falha ao carregar o mapa {MAP_PACKAGE_PATH}!")

    world_settings = world.get_world_settings()
    if not world_settings:
        raise RuntimeError("[MainMenuVerify] ERRO: WorldSettings invalido no mapa carregado!")

    gm_class = world_settings.get_editor_property("default_game_mode")
    if not gm_class or gm_class.get_name() != EXPECTED_GAMEMODE_NAME:
        actual = gm_class.get_name() if gm_class else "None"
        raise RuntimeError(
            f"[MainMenuVerify] ERRO: GameMode Override incorreto! Esperado: {EXPECTED_GAMEMODE_NAME}, Atual: {actual}"
        )

    unreal.log(
        f"[MainMenuVerify] SUCESSO: Mapa {MAP_PACKAGE_PATH} abriu sem erros com GameMode Override = {gm_class.get_path_name()}"
    )


if __name__ == "__main__":
    main()
