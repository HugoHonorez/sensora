function toggleMenu() {
    const sidenav = document.getElementById('sidenav');
    const content = document.getElementById('content');
    
    const isOpen = sidenav.style.left === '0px';

    sidenav.style.left = isOpen ? '-250px' : '0px';
    content.classList.toggle('open', !isOpen);
}
