$(document).ready(function(){
    // Configuración del carrusel
    $('.feedback-slider').owlCarousel({
        loop: false,
        margin: 10,
        nav: true,
        items: 1,
        autoplay: true,
        navText: ["<i class='fas fa-arrow-left'></i>", "<i class='fas fa-arrow-right'></i>"]
    });

    // Detener la animación al redimensionar
    let resizeTimer;
    $(window).resize(function(){
        $(document.body).addClass('resize-animation-stopper');
        clearTimeout(resizeTimer);
        resizeTimer = setTimeout(() => {
            $(document.body).removeClass('resize-animation-stopper');
        }, 400);
    });

    // Mostrar y ocultar la barra de navegación
    $('.navbar-show-btn').click(function(){
        $('.navbar-box').addClass('navbar-box-show');
    });

    $('.navbar-hide-btn').click(function(){
        $('.navbar-box').removeClass("navbar-box-show");
    });


    
    // Cargar y mostrar los datos del CSV en la tabla
    const csvFilePath = 'EjemploRegistro.csv'; 

    fetch(csvFilePath)
        .then(response => response.text())
        .then(data => {
            const rows = data.split('\n');
            const tableBody = document.querySelector('#data-table tbody');

            rows.forEach((row, index) => {
                if (index > 0 && row.trim()) { // Ignorar la primera fila si es de encabezado
                    const columns = row.split(',');
                    const tr = document.createElement('tr');

                    columns.forEach(column => {
                        const td = document.createElement('td');
                        td.textContent = column;
                        tr.appendChild(td);
                    });

                    tableBody.appendChild(tr);
                }
            });
        })
        .catch(error => console.error('Error al cargar el archivo CSV:', error));
});
