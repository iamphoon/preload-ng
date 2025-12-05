#!/bin/bash
# Script de instalação para preload-ng
# Este script compila e opcionalmente instala o preload

set -e

echo "=========================================="
echo "  Preload-NG - Script de Instalação"
echo "=========================================="
echo ""

# Verificar dependências
echo "[1/4] Verificando dependências..."

check_command() {
    if ! command -v "$1" &>/dev/null; then
        echo "Erro: '$1' não encontrado. Por favor, instale-o primeiro."
        exit 1
    fi
}

check_command autoreconf
check_command make
check_command gcc

echo "      ✓ Todas as dependências encontradas"
echo ""

# Executar autoreconf
echo "[2/4] Executando autoreconf..."
autoreconf -fi
echo "      ✓ autoreconf concluído"
echo ""

# Executar configure
echo "[3/4] Executando configure..."
./configure
echo "      ✓ configure concluído"
echo ""

# Compilar
echo "[4/4] Compilando..."
make
echo "      ✓ Compilação concluída"
echo ""

echo "=========================================="
echo "  Compilação finalizada com sucesso!"
echo "=========================================="
echo ""

# Perguntar se deseja instalar
read -p "Deseja instalar o preload? (s/N): " resposta

case "$resposta" in
[sS] | [sS][iI][mM])
    echo ""
    echo "Instalando preload (requer permissões de root)..."
    sudo make install
    echo ""
    echo "✓ Preload instalado com sucesso!"
    echo ""
    echo "Para habilitar o serviço, execute:"
    echo "  sudo systemctl enable preload"
    echo "  sudo systemctl start preload"
    ;;
*)
    echo ""
    echo "Instalação cancelada."
    echo "Para instalar manualmente, execute: sudo make install"
    ;;
esac
