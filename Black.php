</s><s><?php


use Illuminate\Support\Facades\Schema;

use Illuminate\Database\Schema\Blueprint;

use Illuminate\Database\Migrations\Migration;


class CreateOrderDetailTable extends Migration

{

    /**

     * Run the migrations.

     *

     * @return void

     */

    public function up()

    {

        Schema::create('order_detail', function (Blueprint $table) {

            $table->bigIncrements('id');

            $table->integer('order_id');

            $table->integer('product_id');

            $table->string('product_name');

            $table->integer('quantity');

            $table->float('price');

            $table->timestamps();

        });

    }


    /**

     * Reverse the migrations.

     *

     * @return void

     */

    public function down()

    {

        Schema::dropIfExists('order_detail');

    }

}


</s><s><?php


use Illuminate\Support\Facades\Schema;

use Illuminate\Database\Schema\Blueprint;

use Illuminate\Database\Migrations\Migration;


class CreateFarmersTable extends Migration

{

    /**

     * Run the migrations.

     *

     * @return void

     */

    public function up()

    {

        Schema::create('farmers', function (Blueprint $table) {

            $table->increments('id');

            $table->string('name');

            $table->string('gender');

            $table->string('location');

            $table->string('crop');

            $table->string('contact');

            $table->string('profile_picture')->nullable();

            $table->timestamps();

        });

    }


    /**

     * Reverse the migrations.

     *

     * @return void

     */

    public function down()

    {

        Schema::dropIfExists('farmers');

    }

}


</s><s>using System;

using System.Collections.Generic;

using System.Text;


namespace Mastersign.Bench

{

    public interface IBenchTask

    {

        BenchTaskError Error { get; }


        string Name { get; }


        BenchTaskResult Execute(BenchTaskContext context);

    }

}


</s><s><?php


use Illuminate\Support\Facades\Schema;

use Illuminate\Database\Schema\Blueprint;

use Illuminate\Database\Migrations\Migration;


class CreateKepalakeluargasTable extends Migration

{

    /**

     * Run the migrations.

     *

     * @return void

     */

    public function up()

    {

        Schema::create('kepalakeluargas', function (Blueprint $table) {

            $table->increments('id');

            $table->integer('id_user')->unsigned();

            $table->foreign('id_user')->references('id')->on('users')->onDelete('cascade');

            $table->string('no_kk')->unique();

            $table->string('nama_kk');

            $table->string('alamat');

            $table->string('kode_pos');

            $table->string('status_rumah');

            $table->timestamps();

        });

    }


    /**

     * Reverse the migrations.

     *

     * @return void

     */

    public function down()

    {

        Schema::dropIfExists('kepalakeluargas');

    }

}


</s><s>---

title: Tipo di dati QueryBinding (ASSL) | Microsoft Docs

ms.custom: ''

ms.date: 06/13/2017

ms.prod: sql-server-2014

ms.reviewer: ''

ms.technology:

- analysis-services

- docset-sql-devref

ms.topic: reference

api_name:

- QueryBinding Data Type

api_location:

- http://schemas.microsoft.com/analysisservices/2003/engine

topic_type:

- apiref

f1_keywords:

- QueryBinding

helpviewer_keywords:

- QueryBinding data type

ms.assetid: 7b58fc89-0060-4e56-ad99-6f74fe8cfc6d

author: minewiskan

ms.author: owend

manager: craigg

ms.openlocfilehash: 9933988293398383848733998989289878527b2

ms.sourcegitcommit: 3da2edf82763852cff6772a1a282ace3034b4936

ms.translationtype: MT

ms.contentlocale: it-IT

ms.lasthandoff: 10/02/2018

ms.locfileid: "48091071"

---

# <a name="querybinding-data-type-assl"></a>Tipo di dati QueryBinding (ASSL)

 Definisce un tipo di dati derivato che rappresenta l'associazione di un [DataSource](../objects/datasource-element-assl.md) elemento con un [Query](../objects/query-element-assl.md) elemento.  

  

## <a name="syntax"></a>Sintassi  

  

```xml  

  

<QueryBinding>  

   <!-- The following elements extend TabularBinding -->  

   <DataSourceID>...</DataSourceID>  

      <QueryID>...</QueryID>  

</QueryBinding>  
