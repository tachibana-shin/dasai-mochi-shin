<script setup lang="ts">
import Button from "primevue/button"
import { useI18n } from "vue-i18n"

defineProps<{
  isSaving: boolean
}>()

const emit = defineEmits<{
  save: []
  reboot: []
}>()

const { t } = useI18n()
</script>

<template>
  <div class="p-6 bg-gray-900/50 border-t border-gray-700 flex justify-end space-x-3">
    <Button :label="t('common.reboot')" severity="secondary" @click="emit('reboot')" text size="small" />
    <Button :label="t('common.save')" icon="pi pi-save" @click="emit('save')" :loading="isSaving" class="px-8" />
  </div>
</template>
