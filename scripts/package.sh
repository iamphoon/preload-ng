#!/bin/bash
# Script para empacotar preload-ng em tar.gz
# Pergunta a versão e cria o arquivo com nome apropriado

set -e

echo "=========================================="
echo "  Preload-NG - Script de Empacotamento"
echo "=========================================="
echo ""

# Perguntar a versão
read -p "Digite a versão (exemplo: 0.6.6): " versao

# Validar entrada
if [[ -z "$versao" ]]; then
    echo "Erro: Versão não pode estar vazia."
    exit 1
fi

# Validar formato da versão (permitir números e pontos)
if [[ ! "$versao" =~ ^[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo "Aviso: Formato de versão incomum. Continuando..."
fi

# Definir nome do arquivo
nome_arquivo="preload-ng-${versao}.tar.gz"
diretorio_atual=$(basename "$(pwd)")
diretorio_pai=$(dirname "$(pwd)")

echo ""
echo "Criando arquivo: $nome_arquivo"
echo ""

# Ir para o diretório pai e criar o tar.gz
cd "$diretorio_pai"

# Criar o arquivo tar.gz excluindo arquivos desnecessários
tar --exclude='*.o' \
    --exclude='*.a' \
    --exclude='*.so' \
    --exclude='*.la' \
    --exclude='*.lo' \
    --exclude='.deps' \
    --exclude='.libs' \
    --exclude='autom4te.cache' \
    --exclude='config.h' \
    --exclude='config.log' \
    --exclude='config.status' \
    --exclude='stamp-h1' \
    --exclude='libtool' \
    --exclude='*~' \
    --exclude='*.bak' \
    --exclude='*.swp' \
    --exclude='.git' \
    --exclude='.gitignore' \
    -czvf "$nome_arquivo" "$diretorio_atual"

# Mover para o diretório original
mv "$nome_arquivo" "$diretorio_atual/"

cd "$diretorio_atual"

echo ""
echo "=========================================="
echo "  Empacotamento concluído!"
echo "=========================================="
echo ""
echo "Arquivo criado: $(pwd)/$nome_arquivo"
echo "Tamanho: $(du -h "$nome_arquivo" | cut -f1)"
echo ""
