import React from 'react';

import { connect } from 'react-redux';

import SpawnItem from './components/SpawnItem';

import './styles.sass';

import windhelm from './assets/images/windhelm.png';
import whiterun from './assets/images/whiterun.png';
import winterhold from './assets/images/winterhold.png';
import markart from './assets/images/markarth.png';
import solitude from './assets/images/solitude.png';
import dawnstar from './assets/images/dawnstar.png';
import riften from './assets/images/riften.png';
import folkrit from './assets/images/falkreath.png';
import morthal from './assets/images/morthal.png';
import random from './assets/images/random.png';

const getRandomLocationID = () => {
  let locationIDS = ["0001691D", "0001A26F", "00013810", "00016D71", 
                      "00037EDF", "00013A80", "00016BB4", "00013A79", "000138CC"];

  return locationIDS[Math.floor(Math.random() * locationIDS.length)];
}

const spawnItemsList = [
  {
    image: windhelm,
    title: 'Виндхельм',
    description: 'Сепаратистский центр Скайрима, главная база Братьев Бури',
    locationID: "0001691D"
  },
  {
    image: whiterun,
    title: 'Вайтран',
    description: 'Центральный город Скайрима, сердце всей провинции',
    locationID: "0001A26F"
  },
  {
    image: winterhold,
    title: 'Винтерхолд',
    description: 'Старая столица всего Скайрима, важный культурный центр',
    locationID: "00013810"
  },
  {
    image: markart,
    title: 'Маркарт',
    description: 'Город, построенный на руинах древней цивилизации двемеров',
    locationID: "00016D71"
  },
  {
    image: solitude,
    title: 'Солитьюд',
    description: 'Величественный город, который расположен на скале, столица Скайрима',
    locationID: "00037EDF"
  },
  {
    image: dawnstar,
    title: 'Данстар',
    description: 'Холодный уединенный шахтерский городок на севере Скайрима',
    locationID: "00013A80"
  },
  {
    image: riften,
    title: 'Рифтен',
    description: 'Криминальный центр провинции на большом и живописном озере',
    locationID: "00016BB4"
  },
  {
    image: folkrit,
    title: 'Фолкрит',
    description: 'Небольшой уютный и уединенный город с размеренной жизнью',
    locationID: "00013A79"
  },
  {
    image: morthal,
    title: 'Морфал',
    description: 'Изолированный город на Хьялмарских болотах ',
    locationID: "000138CC"
  },
  {
    image: random,
    title: 'Случайный город',
    description: 'Доверьте свою судьбу богам',
    locationID: getRandomLocationID()
  }
]

const SpawnHIVE = (props) => {

  const heading = 'Выберите место появления';
  const question = 'Где вы хотите начать свою историю? Выберите из 9 городов Скайрима';

  const SpawnItemComponents = spawnItemsList.map((item, index) => {
    return <SpawnItem data={item} key={index} />;
  });

  return (
    props.show &&
    <div id="spawn">
      <div className="spawn__inner">
        <h1 className="spawn__heading">{ heading }</h1>
        <p className="spawn__question">{ question }</p>
        <div className="spawn__list">
          { SpawnItemComponents }
        </div>
      </div>
    </div>
  );

}

const mapStateToProps = (state) => {
  const defaultState = state.spawnHiveReducer;

  return {
    show: defaultState.show
  }
}

const mapDispatchToProps = (dispatch) => ({})

export default connect(mapStateToProps, mapDispatchToProps)(SpawnHIVE);