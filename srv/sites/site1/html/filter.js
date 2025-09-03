flatpickr(".datetime", {
    enableTime: true,
    dateFormat: "Y-m-d H:i",
    time_24hr: true
});

const filterRange = document.getElementById('filter-range');
const customFields = document.getElementById('custom-date-fields');

filterRange.addEventListener('change', () => {
    if (filterRange.value === 'custom') {
        customFields.style.display = 'flex';
    } else {
        customFields.style.display = 'none';
    }
});