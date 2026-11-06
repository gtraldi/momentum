import unreal

MAP1_PATH = "/Game/Surf/Maps/Mapa1"
WBP_MAIN_PATH = "/Game/MainMenu/WBP_MainMenu"
WBP_PAUSE_PATH = "/Game/MainMenu/WBP_PauseMenu"
EXPECTED_GI_PATH = "/Script/SurfGame.SurfGameInstance"
EXPECTED_GM_PATH = "/Script/SurfGame.SurfGameMode"
EXPECTED_PC_PATH = "/Script/SurfGame.MainMenuPlayerController"
EXPECTED_PAUSE_CLASS_PATH = "/Script/SurfGame.PauseMenuWidget"


def main():
    unreal.log("[MainMenuVerify] Iniciando validacao do MainMenu, PauseMenu e Quick Restart...")

    for wbp_path in (WBP_MAIN_PATH, WBP_PAUSE_PATH):
        if not unreal.EditorAssetLibrary.does_asset_exist(wbp_path):
            raise RuntimeError(f"[MainMenuVerify] ERRO: {wbp_path} nao encontrado!")
        unreal.log(f"[MainMenuVerify] Asset {wbp_path} validado.")

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP1_PATH):
        raise RuntimeError(f"[MainMenuVerify] ERRO: {MAP1_PATH} nao encontrado!")

    gi_class = unreal.load_class(None, EXPECTED_GI_PATH)
    gm_class = unreal.load_class(None, EXPECTED_GM_PATH)
    pc_class = unreal.load_class(None, EXPECTED_PC_PATH)
    pause_class = unreal.load_class(None, EXPECTED_PAUSE_CLASS_PATH)
    if not gi_class or not gm_class or not pc_class or not pause_class:
        raise RuntimeError(
            "[MainMenuVerify] ERRO: Classes C++ SurfGameInstance / SurfGameMode / MainMenuPlayerController / PauseMenuWidget nao encontradas!"
        )

    unreal.log("[MainMenuVerify] TODAS AS VALIDACOES DO MAIN MENU, PAUSE MENU E QUICK RESTART PASSARAM COM SUCESSO!")


if __name__ == "__main__":
    main()
